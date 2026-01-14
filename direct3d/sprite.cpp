/**
 * @file sprite.cpp
 * @brief スプライト表示の実装
 * @author Natsume Shidara
 * @date 2025/06/12
 * @update 2025/12/10
 */

#include "sprite.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include "DirectXTex.h" // 既存コード準拠
#include "direct3d.h"
#include "shader.h"
#include "debug_ostream.h"
#include "texture.h"

using namespace DirectX;

static constexpr int NUM_VERTEX = 4; // 頂点数

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static float g_ScreenWidth = 0.0f;
static float g_ScreenHeight = 0.0f;

// 頂点構造体
struct Vertex
{
    XMFLOAT3 position; // 頂点座標
    XMFLOAT4 color;    // 色
    XMFLOAT2 uv;       // テクスチャー
};

//=============================================================================
// 内部ヘルパー関数宣言
//=============================================================================
// 共通の描画実行処理（テクスチャ設定以外を行う）
static void SetVertexAndDraw(float dx, float dy, float dw, float dh,
                             float u0, float v0, float u1, float v1,
                             float angle, const XMFLOAT4& color);


//=============================================================================
// 初期化・終了・開始
//=============================================================================
void Sprite_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    // デバイスとデバイスコンテキストのチェック
    if (!pDevice || !pContext)
    {
        hal::dout << "Sprite_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
        return;
    }

    // デバイスとデバイスコンテキストの保存
    g_pDevice = pDevice;
    g_pContext = pContext;

    // 頂点バッファ生成
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(Vertex) * NUM_VERTEX;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);
}

void Sprite_Finalize(void)
{
    SAFE_RELEASE(g_pVertexBuffer);
}

void Sprite_Begin()
{
    // 画面サイズ取得
    g_ScreenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    g_ScreenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());

    // 頂点シェーダーにプロジェクション行列（2D用平行投影）を設定
    Shader_SetProjectionMatrix(XMMatrixOrthographicOffCenterLH(0.0f, g_ScreenWidth, g_ScreenHeight, 0.0f, 0.0f, 1.0f));

    // シェーダーを開始（共通設定）
    Shader_Begin();
}

//=============================================================================
// 描画関数群（Texture ID版）
//=============================================================================

void Sprite_Draw(int texid, float display_x, float display_y, float angle, const XMFLOAT4& color)
{
    float w = static_cast<float>(Texture_Width(texid));
    float h = static_cast<float>(Texture_Height(texid));
    Sprite_Draw(texid, display_x, display_y, 0.0f, 0.0f, w, h, w, h, angle, color);
}

void Sprite_Draw(int texid, float display_x, float display_y, float display_w, float display_h, float angle, const XMFLOAT4& color)
{
    float w = static_cast<float>(Texture_Width(texid));
    float h = static_cast<float>(Texture_Height(texid));
    Sprite_Draw(texid, display_x, display_y, 0.0f, 0.0f, w, h, display_w, display_h, angle, color);
}

void Sprite_Draw(int texid, float display_x, float display_y, float uvcut_x, float uvcut_y, float uvcut_w, float uvcut_h, float angle, const XMFLOAT4& color)
{
    Sprite_Draw(texid, display_x, display_y, uvcut_x, uvcut_y, uvcut_w, uvcut_h, uvcut_w, uvcut_h, angle, color);
}

// 最も詳細な引数を持つメイン描画関数（ID版）
void Sprite_Draw(int texid, float display_x, float display_y, float uvcut_x, float uvcut_y, float uvcut_w, float uvcut_h, float display_w, float display_h, float angle, const XMFLOAT4& color)
{
    if (texid < 0) return;

    // 1. テクスチャをバインド（Textureシステムの管理下にあるもの）
    Texture_SetTexture(texid);

    // 2. UV座標計算
    const float imgW = static_cast<float>(Texture_Width(texid));
    const float imgH = static_cast<float>(Texture_Height(texid));

    // ゼロ除算防止
    if (imgW == 0.0f || imgH == 0.0f) return;

    float u0 = uvcut_x / imgW;
    float v0 = uvcut_y / imgH;
    float u1 = (uvcut_x + uvcut_w) / imgW;
    float v1 = (uvcut_y + uvcut_h) / imgH;

    // 3. 共通描画処理へ
    SetVertexAndDraw(display_x, display_y, display_w, display_h, u0, v0, u1, v1, angle, color);
}

//=============================================================================
// 描画関数（SRV版）
//=============================================================================
void Sprite_Draw(ID3D11ShaderResourceView* pSRV, float display_x, float display_y, float display_w, float display_h, float angle, const DirectX::XMFLOAT4& color)
{
    if (!pSRV) return;

    // 1. テクスチャを直接バインド（スロット0と仮定）
    g_pContext->PSSetShaderResources(0, 1, &pSRV);

    // 2. UV座標計算（全体を描画するので 0.0 ～ 1.0）
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;

    // 3. 共通描画処理へ
    SetVertexAndDraw(display_x, display_y, display_w, display_h, u0, v0, u1, v1, angle, color);
}


//=============================================================================
// 内部ヘルパー実装
//=============================================================================
static void SetVertexAndDraw(float dx, float dy, float dw, float dh,
                             float u0, float v0, float u1, float v1,
                             float angle, const XMFLOAT4& color)
{
    // 頂点バッファをロックする
    D3D11_MAPPED_SUBRESOURCE msr;
    HRESULT hr = g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    if (FAILED(hr)) return;

    // 頂点バッファへの仮想ポインタを取得
    Vertex* v = static_cast<Vertex*>(msr.pData);

    // 頂点座標設定（中心基準のオフセット）
    // 左上(-0.5, -0.5) ～ 右下(+0.5, +0.5) の矩形を作る
    v[0].position = { -0.5f, -0.5f, 0.0f }; // LT
    v[1].position = { +0.5f, -0.5f, 0.0f }; // RT
    v[2].position = { -0.5f, +0.5f, 0.0f }; // LB
    v[3].position = { +0.5f, +0.5f, 0.0f }; // RB

    // 色設定
    v[0].color = color;
    v[1].color = color;
    v[2].color = color;
    v[3].color = color;

    // UV設定
    v[0].uv = { u0, v0 };
    v[1].uv = { u1, v0 };
    v[2].uv = { u0, v1 };
    v[3].uv = { u1, v1 };

    // 頂点バッファのロックを解除
    g_pContext->Unmap(g_pVertexBuffer, 0);

    // ワールド行列計算
    // 拡大縮小 -> 回転 -> 平行移動
    XMMATRIX mat = XMMatrixTransformation2D(
        XMVectorSet(0, 0, 0, 0),    // ScalingOrigin
        0.0f,                       // ScalingOrientation
        XMVectorSet(dw, dh, 0, 0),  // Scaling (描画サイズ)
        XMVectorSet(0, 0, 0, 0),    // RotationOrigin
        angle,                      // Rotation
        XMVectorSet(dx + dw / 2.0f, dy + dh / 2.0f, 0, 0) // Translation (中心座標へ移動)
    );

    Shader_SetWorldMatrix(mat);

    // 描画パイプライン設定
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 描画
    g_pContext->Draw(NUM_VERTEX, 0);
}