/****************************************
 * @file cube.cpp
 * @brief 立方体描画モジュール実装（UV修正・深度修正版）
 * @author Natsume Shidara
 * @date 2025/12/10
 ****************************************/

#include "cube.h"
#include "direct3d.h"
#include "shader3d.h"
#include "shader3d_unlit.h"
#include "texture.h"
#include "debug_ostream.h"
#include "shader_shadow_map.h"

using namespace DirectX;

//=============================================================================
// 内部構造体・定数
//=============================================================================

struct Vertex {
    XMFLOAT3 position; // 位置
    XMFLOAT3 normal;   // 法線
    XMFLOAT4 color;    // 頂点カラー
    XMFLOAT2 uv;       // テクスチャ座標
};

static const int NUM_VERTICES = 36;

//=============================================================================
// 内部変数
//=============================================================================
static ID3D11Buffer* g_pVertexBuffer = nullptr;
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static const float CUBE_SIZE = 1.0f;
static const float HALF_SIZE = CUBE_SIZE / 2.0f;

//=============================================================================
// 初期化
//=============================================================================
void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    g_pDevice = pDevice;
    g_pContext = pContext;

    // テクスチャアトラス(box.jpg)のUV計算
    // 全体2048x2048, 1マス512x512 なので 4x4 グリッド
    float w = 1.0f / 4.0f; // 0.25
    float h = 1.0f / 4.0f; // 0.25

    // 各面のUVオフセット (左上座標)
    // Row 0: 1, 2, 3, 4
    XMFLOAT2 uv1 = { w * 0, h * 0 }; // Front (1)
    XMFLOAT2 uv2 = { w * 1, h * 0 }; // Back  (2)
    XMFLOAT2 uv3 = { w * 2, h * 0 }; // Top   (3)
    XMFLOAT2 uv4 = { w * 3, h * 0 }; // Bottom(4)

    // Row 1: 5, 6
    XMFLOAT2 uv5 = { w * 0, h * 1 }; // Left  (5)
    XMFLOAT2 uv6 = { w * 1, h * 1 }; // Right (6)

    Vertex vertices[NUM_VERTICES] = {
        // --- 1. 前面 (Z-) : Box 1 ---
        // 左上, 右上, 左下
        { {-HALF_SIZE,  HALF_SIZE, -HALF_SIZE}, {0,0,-1}, {1,1,1,1}, {uv1.x, uv1.y} },
        { { HALF_SIZE,  HALF_SIZE, -HALF_SIZE}, {0,0,-1}, {1,1,1,1}, {uv1.x + w, uv1.y} },
        { {-HALF_SIZE, -HALF_SIZE, -HALF_SIZE}, {0,0,-1}, {1,1,1,1}, {uv1.x, uv1.y + h} },
        // 右上, 右下, 左下
        { { HALF_SIZE,  HALF_SIZE, -HALF_SIZE}, {0,0,-1}, {1,1,1,1}, {uv1.x + w, uv1.y} },
        { { HALF_SIZE, -HALF_SIZE, -HALF_SIZE}, {0,0,-1}, {1,1,1,1}, {uv1.x + w, uv1.y + h} },
        { {-HALF_SIZE, -HALF_SIZE, -HALF_SIZE}, {0,0,-1}, {1,1,1,1}, {uv1.x, uv1.y + h} },

        // --- 2. 背面 (Z+) : Box 2 ---
        // 裏側から見て正しくなるように左右反転配置
        { { HALF_SIZE,  HALF_SIZE,  HALF_SIZE}, {0,0,1},  {1,1,1,1}, {uv2.x, uv2.y} },       // 右上(裏から見て左上)
        { {-HALF_SIZE,  HALF_SIZE,  HALF_SIZE}, {0,0,1},  {1,1,1,1}, {uv2.x + w, uv2.y} },   // 左上(裏から見て右上)
        { { HALF_SIZE, -HALF_SIZE,  HALF_SIZE}, {0,0,1},  {1,1,1,1}, {uv2.x, uv2.y + h} },   // 右下(裏から見て左下)

        { {-HALF_SIZE,  HALF_SIZE,  HALF_SIZE}, {0,0,1},  {1,1,1,1}, {uv2.x + w, uv2.y} },   // 左上
        { {-HALF_SIZE, -HALF_SIZE,  HALF_SIZE}, {0,0,1},  {1,1,1,1}, {uv2.x + w, uv2.y + h} }, // 左下(裏から見て右下)
        { { HALF_SIZE, -HALF_SIZE,  HALF_SIZE}, {0,0,1},  {1,1,1,1}, {uv2.x, uv2.y + h} },   // 右下

        // --- 3. 上面 (Y+) : Box 3 ---
        { {-HALF_SIZE,  HALF_SIZE,  HALF_SIZE}, {0,1,0},  {1,1,1,1}, {uv3.x, uv3.y} },
        { { HALF_SIZE,  HALF_SIZE,  HALF_SIZE}, {0,1,0},  {1,1,1,1}, {uv3.x + w, uv3.y} },
        { {-HALF_SIZE,  HALF_SIZE, -HALF_SIZE}, {0,1,0},  {1,1,1,1}, {uv3.x, uv3.y + h} },

        { { HALF_SIZE,  HALF_SIZE,  HALF_SIZE}, {0,1,0},  {1,1,1,1}, {uv3.x + w, uv3.y} },
        { { HALF_SIZE,  HALF_SIZE, -HALF_SIZE}, {0,1,0},  {1,1,1,1}, {uv3.x + w, uv3.y + h} },
        { {-HALF_SIZE,  HALF_SIZE, -HALF_SIZE}, {0,1,0},  {1,1,1,1}, {uv3.x, uv3.y + h} },

        // --- 4. 下面 (Y-) : Box 4 ---
        { {-HALF_SIZE, -HALF_SIZE, -HALF_SIZE}, {0,-1,0}, {1,1,1,1}, {uv4.x, uv4.y} },
        { { HALF_SIZE, -HALF_SIZE, -HALF_SIZE}, {0,-1,0}, {1,1,1,1}, {uv4.x + w, uv4.y} },
        { {-HALF_SIZE, -HALF_SIZE,  HALF_SIZE}, {0,-1,0}, {1,1,1,1}, {uv4.x, uv4.y + h} },

        { { HALF_SIZE, -HALF_SIZE, -HALF_SIZE}, {0,-1,0}, {1,1,1,1}, {uv4.x + w, uv4.y} },
        { { HALF_SIZE, -HALF_SIZE,  HALF_SIZE}, {0,-1,0}, {1,1,1,1}, {uv4.x + w, uv4.y + h} },
        { {-HALF_SIZE, -HALF_SIZE,  HALF_SIZE}, {0,-1,0}, {1,1,1,1}, {uv4.x, uv4.y + h} },

        // --- 5. 左面 (X-) : Box 5 ---
        // 左面も裏側（外側）から見て正しくなるように考慮
        { {-HALF_SIZE,  HALF_SIZE,  HALF_SIZE}, {-1,0,0}, {1,1,1,1}, {uv5.x, uv5.y} },     // 奥上
        { {-HALF_SIZE,  HALF_SIZE, -HALF_SIZE}, {-1,0,0}, {1,1,1,1}, {uv5.x + w, uv5.y} }, // 手前上
        { {-HALF_SIZE, -HALF_SIZE,  HALF_SIZE}, {-1,0,0}, {1,1,1,1}, {uv5.x, uv5.y + h} }, // 奥下

        { {-HALF_SIZE,  HALF_SIZE, -HALF_SIZE}, {-1,0,0}, {1,1,1,1}, {uv5.x + w, uv5.y} }, // 手前上
        { {-HALF_SIZE, -HALF_SIZE, -HALF_SIZE}, {-1,0,0}, {1,1,1,1}, {uv5.x + w, uv5.y + h} }, // 手前下
        { {-HALF_SIZE, -HALF_SIZE,  HALF_SIZE}, {-1,0,0}, {1,1,1,1}, {uv5.x, uv5.y + h} }, // 奥下

        // --- 6. 右面 (X+) : Box 6 ---
        { { HALF_SIZE,  HALF_SIZE, -HALF_SIZE}, {1,0,0},  {1,1,1,1}, {uv6.x, uv6.y} },
        { { HALF_SIZE,  HALF_SIZE,  HALF_SIZE}, {1,0,0},  {1,1,1,1}, {uv6.x + w, uv6.y} },
        { { HALF_SIZE, -HALF_SIZE, -HALF_SIZE}, {1,0,0},  {1,1,1,1}, {uv6.x, uv6.y + h} },

        { { HALF_SIZE,  HALF_SIZE,  HALF_SIZE}, {1,0,0},  {1,1,1,1}, {uv6.x + w, uv6.y} },
        { { HALF_SIZE, -HALF_SIZE,  HALF_SIZE}, {1,0,0},  {1,1,1,1}, {uv6.x + w, uv6.y + h} },
        { { HALF_SIZE, -HALF_SIZE, -HALF_SIZE}, {1,0,0},  {1,1,1,1}, {uv6.x, uv6.y + h} },
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * NUM_VERTICES;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = vertices;

    HRESULT hr = g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);
    if (FAILED(hr))
    {
        hal::dout << "Cube_Initialize: Vertex Buffer Creation Failed" << std::endl;
    }
}

//=============================================================================
// 終了
//=============================================================================
void Cube_Finalize()
{
    if (g_pVertexBuffer)
    {
        g_pVertexBuffer->Release();
        g_pVertexBuffer = nullptr;
    }
}

//=============================================================================
// 更新
//=============================================================================
void Cube_Update(double elapsed_time)
{
    (void)elapsed_time;
}

//=============================================================================
// 描画 (通常)
//=============================================================================
void Cube_Draw(int texID, const XMMATRIX& world, const XMFLOAT4& color)
{
    if (!g_pContext || !g_pVertexBuffer) return;

    //  深度テストを有効化
    Direct3D_DepthStencilStateDepthIsEnable(true);

    // シェーダー設定
    Shader3D_Begin();
    Shader3D_SetWorldMatrix(world);
    Shader3D_SetColor(color);

    // テクスチャ設定
    Texture_SetTexture(texID);

    // 頂点バッファ設定
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    // プリミティブトポロジ設定
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 描画
    g_pContext->Draw(NUM_VERTICES, 0);
}

//=============================================================================
// 描画 (影生成用)
//=============================================================================
void Cube_DrawShadow(const DirectX::XMMATRIX& world)
{
    if (!g_pContext || !g_pVertexBuffer) return;

    // 影生成時も深度書き込みは必須
    Direct3D_DepthStencilStateDepthIsEnable(true);

    // シャドウマップ用シェーダーに行列をセット
    ShaderShadowMap_SetWorldMatrix(world);

    // 頂点バッファ設定
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 描画
    g_pContext->Draw(NUM_VERTICES, 0);
}

//=============================================================================
// AABB取得
//=============================================================================
AABB Cube_GetAABB(const XMFLOAT3& position)
{
    AABB aabb;
    aabb.min = { position.x - HALF_SIZE, position.y - HALF_SIZE, position.z - HALF_SIZE };
    aabb.max = { position.x + HALF_SIZE, position.y + HALF_SIZE, position.z + HALF_SIZE };
    return aabb;
}