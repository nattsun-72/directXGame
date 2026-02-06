/****************************************
 * billboard.cpp
 *
 * 概要:
 *   ビルボード描画モジュール
 *   - カメラに常に正面を向く平面を描画
 *   - テクスチャ付き
 *
 * 主な機能:
 *   Billboard_Initialize() : 初期化
 *   Billboard_Finalize()   : 解放
 *   Billboard_Update(...)  : 更新
 *   Billboard_Draw(...)    : 描画
 ****************************************/
#include "billboard.h"
#include <DirectXMath.h>
#include "color.h"
#include "debug_ostream.h"
#include "direct3d.h"
#include "player_camera.h"
#include "shader_billboard.h"
#include "texture.h"

using namespace DirectX;

//======================================
// 定数・グローバル変数
//======================================
static constexpr int NUM_VERTEX = 4; // ビルボードは4頂点の矩形

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ

// 初期化時に外部から渡されるデバイス・コンテキスト
// Release不要
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static XMFLOAT4X4 g_mtxView{};

// 頂点構造体
struct Vertex3d
{
    XMFLOAT3 position; // 頂点座標
    XMFLOAT4 color; // 頂点カラー
    XMFLOAT2 uv; // テクスチャUV
};

//======================================
// ビルボード頂点データ(4頂点)
// 中心を原点とした正方形
//======================================
static Vertex3d g_BillboardVertex[] = {
    // 左上
    { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
    // 右上
    { { 0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
    // 左下
    { { -0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
    // 右下
    { { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
};

//======================================
// カメラ正面向き行列を取得
//======================================
static XMMATRIX GetBillboardMatrix()
{
    // ビュー行列から回転成分のみを抽出し、その逆行列を求める
    XMMATRIX view = XMLoadFloat4x4(&g_mtxView);

    // ビュー行列の回転部分(3x3)を抽出
    XMVECTOR right = XMVectorSet(XMVectorGetX(view.r[0]), XMVectorGetX(view.r[1]), XMVectorGetX(view.r[2]), 0.0f);

    XMVECTOR up = XMVectorSet(XMVectorGetY(view.r[0]), XMVectorGetY(view.r[1]), XMVectorGetY(view.r[2]), 0.0f);

    XMVECTOR forward = XMVectorSet(XMVectorGetZ(view.r[0]), XMVectorGetZ(view.r[1]), XMVectorGetZ(view.r[2]), 0.0f);

    // ビルボード行列を構築(カメラの逆向き)
    XMMATRIX billboard;
    billboard.r[0] = right;
    billboard.r[1] = up;
    billboard.r[2] = forward;
    billboard.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    return billboard;
}

//======================================
// 初期化処理
//======================================
void Billboard_Initialize()
{
    hal::dout << "Billboard_Initialize: begin" << std::endl;

    // デバイス・コンテキストの妥当性チェック
    if (!Direct3D_GetDevice() || !Direct3D_GetContext())
    {
        hal::dout << "Billboard_Initialize() : 無効なデバイスまたはコンテキスト" << std::endl;
        return;
    }

    g_pDevice = Direct3D_GetDevice();
    g_pContext = Direct3D_GetContext();

    // 頂点バッファ生成設定
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT; // 静的バッファ
    bd.ByteWidth = sizeof(Vertex3d) * NUM_VERTEX;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    bd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = g_BillboardVertex;
    sd.SysMemPitch = 0;
    sd.SysMemSlicePitch = 0;

    // 頂点バッファ生成
    HRESULT hr = g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);
    if (FAILED(hr))
    {
        hal::dout << "Billboard_Initialize: 頂点バッファの作成に失敗 hr=0x" << std::hex << hr << std::endl;
        return;
    }

    hal::dout << "Billboard_Initialize: success" << std::endl;
}

//======================================
// 解放処理
//======================================
void Billboard_Finalize()
{
    hal::dout << "Billboard_Finalize: releasing resources" << std::endl;
    SAFE_RELEASE(g_pVertexBuffer);

    g_pDevice = nullptr;
    g_pContext = nullptr;
}

//======================================
// 更新処理
//======================================
void Billboard_Update(double deltaTime)
{
    (void)deltaTime;
    // 現在は特に更新処理なし
    // アニメーションなどが必要な場合はここに実装
}

//======================================
// 描画処理
//======================================
void Billboard_Draw(int texId, const DirectX::XMFLOAT3& position, const XMFLOAT2& scale, const XMFLOAT2& pivot,const XMFLOAT4& color)
{

    //Shader_Billboard_SetUVParameter({ { 0.2f, 0.5f }, { 0.2f * 3.0f, 0.5f * 2.0f } });

    Shader_Billboard_SetUVParameter({ { 1.0f, 1.0f }, { 0.0f, 0.0f } });
    if (!g_pContext || !g_pVertexBuffer)
    {
        hal::dout << "Billboard_Draw: 無効なコンテキストまたは頂点バッファ" << std::endl;
        return;
    }

    // シェーダーを開始
    Shader_Billboard_Begin();

    // カラーを設定(現在のピクセルシェーダーでは使用されないが、互換性のため)
    Shader_Billboard_SetColor(color);

    // 深度テスト有効(深度書き込みは無効にすることを推奨)
    Direct3D_DepthStencilStateDepthIsEnable(true);

    // テクスチャをセット
    Texture_SetTexture(texId);

    // 頂点バッファを描画パイプラインに設定
    UINT stride = sizeof(Vertex3d);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    // プリミティブトポロジ設定(トライアングルストリップで4頂点から2三角形を描画)
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // カメラ正面向き行列を取得
    XMMATRIX billboardMatrix = GetBillboardMatrix();

    // ワールド変換行列の作成
    // スケール → ピボットオフセット → ビルボード回転 → 平行移動の順
    XMMATRIX matScale = XMMatrixScaling(scale.x, scale.y, 1.0f);
    XMMATRIX pivotOffset = XMMatrixTranslation(-pivot.x, -pivot.y, 0.0f);
    XMMATRIX matTranslation = XMMatrixTranslation(position.x, position.y, position.z);
    XMMATRIX matWorld = matScale * pivotOffset * billboardMatrix * matTranslation;

    // ワールド変換行列を頂点シェーダーに設定
    Shader_Billboard_SetWorldMatrix(matWorld);

    // 描画実行(4頂点でトライアングルストリップ)
    g_pContext->Draw(NUM_VERTEX, 0);
}

void Billboard_Draw(int texId, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT4& tex_cut, const DirectX::XMFLOAT2& pivot, const XMFLOAT4& color)
{

    float uv_x = static_cast<float>(tex_cut.x / Texture_Width(texId));
    float uv_y = static_cast<float>(tex_cut.y / Texture_Height(texId));
    float uv_w = static_cast<float>(tex_cut.z / Texture_Width(texId));
    float uv_h = static_cast<float>(tex_cut.w / Texture_Height(texId));

    Shader_Billboard_SetUVParameter({ { uv_w, uv_h }, { uv_x, uv_y } });

    if (!g_pContext || !g_pVertexBuffer)
    {
        hal::dout << "Billboard_Draw: 無効なコンテキストまたは頂点バッファ" << std::endl;
        return;
    }

    // シェーダーを開始
    Shader_Billboard_Begin();

    // カラーを設定(現在のピクセルシェーダーでは使用されないが、互換性のため)
    Shader_Billboard_SetColor(color);

    // 深度テスト有効(深度書き込みは無効にすることを推奨)
    Direct3D_DepthStencilStateDepthIsEnable(true);

    // テクスチャをセット
    Texture_SetTexture(texId);

    // 頂点バッファを描画パイプラインに設定
    UINT stride = sizeof(Vertex3d);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    // プリミティブトポロジ設定(トライアングルストリップで4頂点から2三角形を描画)
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // カメラ正面向き行列を取得
    XMMATRIX billboardMatrix = GetBillboardMatrix();

    // ワールド変換行列の作成
    // スケール → ピボットオフセット → ビルボード回転 → 平行移動の順
    XMMATRIX matScale = XMMatrixScaling(scale.x, scale.y, 1.0f);
    XMMATRIX pivotOffset = XMMatrixTranslation(-pivot.x, -pivot.y, 0.0f);
    XMMATRIX matTranslation = XMMatrixTranslation(position.x, position.y, position.z);
    XMMATRIX matWorld = matScale * pivotOffset * billboardMatrix * matTranslation;

    // ワールド変換行列を頂点シェーダーに設定
    Shader_Billboard_SetWorldMatrix(matWorld);

    // 描画実行(4頂点でトライアングルストリップ)
    g_pContext->Draw(NUM_VERTEX, 0);
}

void Billboard_SetViewMatrix(const DirectX::XMFLOAT4X4& view)
{
    g_mtxView = view;
    g_mtxView._41 = g_mtxView._42 = g_mtxView._43 = 0.0f;
}
