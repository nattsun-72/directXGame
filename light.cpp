/****************************************
 * @file light.cpp
 * @brief ライト制御モジュール（ハイブリッド型）
 * @detail
 * - 環境光・平行光・鏡面光・点光源対応
 * - 内部状態を保持し、毎フレームGPUへ一括転送可能
 * - HLSL定数バッファ構造体と整合
 * @author Natsume Shidara
 * @date 2025/10/29
 * @update 2025/12/19 (Refactored: 警告修正およびコメント形式の統一)
 ****************************************/

#include "light.h"
#include <cassert>
#include <algorithm>

 //--------------------------------------
 // 静的メンバ：DirectXリソース群
 //--------------------------------------
 // DirectXデバイス、コンテキスト、および各シェーダスロット用定数バッファを管理
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static ID3D11Buffer* g_pPSConstantBuffer1 = nullptr; // 環境光用 (b1)
static ID3D11Buffer* g_pPSConstantBuffer2 = nullptr; // 平行光用 (b2)
static ID3D11Buffer* g_pPSConstantBuffer3 = nullptr; // 鏡面光用 (b3)
static ID3D11Buffer* g_pPSConstantBuffer4 = nullptr; // 点光源用 (b4)

//--------------------------------------
// 内部ライト状態（CPU側キャッシュ）
//--------------------------------------
// GPUに送信するデータのミラーを保持。Update時に一括転送を行う。
static AmbientCB g_ambientData = { XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f) };
static DirectionalLightCB g_directionalData = {
    XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f), // 下向きの平行光
    XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)   // 白色
};
static SpecularLightCB g_specularData = {
    XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), // 鏡面反射色
    XMFLOAT3(0.0f, 0.0f, -5.0f),      // カメラ位置（初期値）
    32.0f                             // 光沢度
};
static PointLightListCB g_pointLightData = { {}, 0, XMFLOAT3(0.0f, 0.0f, 0.0f) };

//======================================
// 内部ヘルパー関数
//======================================
/**
 * @brief 定数バッファを生成するユーティリティ関数
 * @param[in] size バッファサイズ（構造体サイズ）
 * @return 生成された ID3D11Buffer*
 * @detail 16バイトアライメントを保証してバッファを生成する。
 */
static ID3D11Buffer* CreateConstantBuffer(UINT size)
{
    D3D11_BUFFER_DESC desc = {};
    // 16バイト境界に切り上げる計算
    desc.ByteWidth = static_cast<UINT>((size + 15) / 16 * 16);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    ID3D11Buffer* buffer = nullptr;
    HRESULT hr = g_pDevice->CreateBuffer(&desc, nullptr, &buffer);

    // 開発時のエラー検知
    assert(SUCCEEDED(hr) && "ライト定数バッファ生成失敗");
    (void)hr;

    return buffer;
}

//======================================
// 初期化・終了処理
//======================================
/**
 * @brief ライトシステムの初期化
 * @param[in] pDevice  Direct3Dデバイス
 * @param[in] pContext デバイスコンテキスト
 * @detail 必要な定数バッファをすべて生成し、静的ポインタに格納する。
 */
void Light_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    assert(pDevice && pContext);
    g_pDevice = pDevice;
    g_pContext = pContext;

    // 各定数バッファの生成
    g_pPSConstantBuffer1 = CreateConstantBuffer(sizeof(AmbientCB));
    g_pPSConstantBuffer2 = CreateConstantBuffer(sizeof(DirectionalLightCB));
    g_pPSConstantBuffer3 = CreateConstantBuffer(sizeof(SpecularLightCB));
    g_pPSConstantBuffer4 = CreateConstantBuffer(sizeof(PointLightListCB));
}

/**
 * @brief ライトシステムの終了処理
 * @detail リソースの安全な解放とポインタの初期化を行う。
 */
void Light_Finalize(void)
{
    // COMオブジェクトの解放用ラムダ
    auto SafeRelease = [](ID3D11Buffer*& buf)
        {
            if (buf)
            {
                buf->Release();
                buf = nullptr;
            }
        };

    SafeRelease(g_pPSConstantBuffer1);
    SafeRelease(g_pPSConstantBuffer2);
    SafeRelease(g_pPSConstantBuffer3);
    SafeRelease(g_pPSConstantBuffer4);

    g_pDevice = nullptr;
    g_pContext = nullptr;
}

//======================================
// 基本設定関数群
//======================================
/**
 * @brief 環境光の設定
 * @param[in] color RGBカラー
 */
void Light_SetAmbient(const XMFLOAT3& color)
{
    g_ambientData.ambient_color = XMFLOAT4(color.x, color.y, color.z, 1.0f);
}

/**
 * @brief 平行光の設定
 * @param[in] world_directional 照射方向ベクトル
 * @param[in] color             光の色
 */
void Light_SetDirectional_World(const XMFLOAT4& world_directional, const XMFLOAT4& color)
{
    g_directionalData.directonal_world_vector = world_directional;
    g_directionalData.directonal_color = color;
}

/**
 * @brief 鏡面光の設定
 * @param[in] color          鏡面色
 * @param[in] CameraPosition カメラのワールド座標
 * @param[in] power          反射強度
 */
void Light_SetSpecular(const XMFLOAT4& color, const XMFLOAT3& CameraPosition, float power)
{
    g_specularData.specular_color = color;
    g_specularData.eye_posW = CameraPosition;
    g_specularData.specular_power = power;
}

/**
 * @brief 全点光源データの一括設定
 * @param[in] lights 点光源データ配列
 * @param[in] count  光源数
 */
void Light_SetPointLight(const PointLightCB* lights, int count)
{
    if (!lights) return;

    /* 負数入力の防止と最大数制限 */
    UINT safeCount = static_cast<UINT>(max(0, count));
    g_pointLightData.pointLightCount = min(safeCount, 4U);

    for (UINT i = 0; i < static_cast<UINT>(g_pointLightData.pointLightCount); ++i)
    {
        g_pointLightData.pointLight[i] = lights[i];
    }
}

//======================================
// 点光源操作群
//======================================
/**
 * @brief 点光源の追加
 * @param[in] pos   ワールド座標
 * @param[in] color 色
 * @param[in] range 減衰距離
 */
void Light_AddPointLight(const XMFLOAT3& pos, const XMFLOAT4& color, float range)
{
    /* 最大数（4個）に達している場合は何もしない */
    if (g_pointLightData.pointLightCount >= 4)
    {
        return;
    }

    UINT idx = g_pointLightData.pointLightCount++;
    g_pointLightData.pointLight[idx].posW = pos;
    g_pointLightData.pointLight[idx].color = color;
    g_pointLightData.pointLight[idx].range = range;
}

/**
 * @brief 点光源の位置更新
 * @param[in] index ライトインデックス
 * @param[in] pos   新しいワールド座標
 */
void Light_SetPointLightPosition(UINT index, const XMFLOAT3& pos)
{
    /* 引数をUINTに変更し、符号比較警告を解消 */
    if (index >= static_cast<UINT>(g_pointLightData.pointLightCount))
    {
        return;
    }
    g_pointLightData.pointLight[index].posW = pos;
}

/**
 * @brief 点光源の色更新
 * @param[in] index ライトインデックス
 * @param[in] color 新しい色
 */
void Light_SetPointLightColor(UINT index, const XMFLOAT4& color)
{
    if (index >= static_cast<UINT>(g_pointLightData.pointLightCount))
    {
        return;
    }
    g_pointLightData.pointLight[index].color = color;
}

/**
 * @brief 点光源の範囲更新
 * @param[in] index ライトインデックス
 * @param[in] range 新しい減衰距離
 */
void Light_SetPointLightRange(UINT index, float range)
{
    if (index >= static_cast<UINT>(g_pointLightData.pointLightCount))
    {
        return;
    }
    g_pointLightData.pointLight[index].range = range;
}

/**
 * @brief 全点光源のクリア
 */
void Light_ClearPointLights()
{
    g_pointLightData.pointLightCount = 0;
}

//======================================
// GPU更新処理
//======================================
/**
 * @brief GPUへの全ライト情報転送
 * @param[in] elapsed_time 経過時間（未使用）
 * @detail キャッシュデータを各定数バッファへ転送し、PSのb1-b4スロットにバインドする。
 */
void Light_Update(double elapsed_time)
{
    (void)elapsed_time; // 未使用引数の警告抑制

    if (!g_pContext) return;

    /* サブリソースの更新（一括転送） */
    g_pContext->UpdateSubresource(g_pPSConstantBuffer1, 0, nullptr, &g_ambientData, 0, 0);
    g_pContext->UpdateSubresource(g_pPSConstantBuffer2, 0, nullptr, &g_directionalData, 0, 0);
    g_pContext->UpdateSubresource(g_pPSConstantBuffer3, 0, nullptr, &g_specularData, 0, 0);
    g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &g_pointLightData, 0, 0);

    /* ピクセルシェーダのレジスタb1～b4へ一括セット */
    ID3D11Buffer* psCBuffers[] = {
        g_pPSConstantBuffer1,
        g_pPSConstantBuffer2,
        g_pPSConstantBuffer3,
        g_pPSConstantBuffer4
    };
    g_pContext->PSSetConstantBuffers(1, 4, psCBuffers);
}