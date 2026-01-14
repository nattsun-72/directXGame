/**
 * @file grid.cpp
 * @brief XZ平面グリッドの表示
 * 
 * @author Natsume Shidara
 * @date 2025/09/11
 */

#include "grid.h"

#include <DirectXMath.h>
#include "debug_ostream.h"
#include "direct3d.h"
#include "shader3d.h"
#include "color.h"
#include "texture.h"

using namespace DirectX;

static constexpr int GRID_H_COUNT = 10; // X Axis
static constexpr int GRID_V_COUNT = 10; // Z Axis
static constexpr int GRID_H_LINE_COUNT = GRID_H_COUNT + 1;
static constexpr int GRID_V_LINE_COUNT = GRID_V_COUNT + 1;
static constexpr int NUM_VERTEX = (GRID_H_LINE_COUNT + GRID_V_LINE_COUNT) * 2;

static constexpr float GRID_SIZE = 1.0f;
static constexpr float GRID_H_START = (GRID_H_LINE_COUNT * GRID_SIZE) * 0.5f;
static constexpr float GRID_V_START = (GRID_V_LINE_COUNT * GRID_SIZE) * 0.5f;

static constexpr float GRID_HALF_EXTENT_X = (GRID_H_COUNT * GRID_SIZE) * 0.5f;
static constexpr float GRID_HALF_EXTENT_Z = (GRID_V_COUNT * GRID_SIZE) * 0.5f;

static constexpr Color::COLOR GRID_H_COLOR = Color::NEON_GREEN;
static constexpr Color::COLOR GRID_V_COLOR = Color::RED;

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static int g_GridTexId = -1;
static std::wstring TEXTURE_PATH = L"assets/white.png";

// 頂点構造体
struct Vertex3d
{
    XMFLOAT3 position; // 頂点座標
    XMFLOAT4 color; // 色
};

static Vertex3d g_GridVertex[NUM_VERTEX]{};

void Grid_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    // デバイスとデバイスコンテキストのチェック
    if (!pDevice || !pContext)
    {
        hal::dout << "Polygon_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
        return;
    }

    g_pDevice = pDevice;
    g_pContext = pContext;

    for (int i = 0; i < GRID_V_LINE_COUNT; i++)
    {
        int index_offset = 0;

        float x1 = -GRID_HALF_EXTENT_X;
        float x2 = +GRID_HALF_EXTENT_X;

        float z1 = -GRID_HALF_EXTENT_Z + GRID_SIZE * i;
        float z2 = z1;

        g_GridVertex[i * 2 + index_offset].position = { x1, 0.0f, z1 };
        g_GridVertex[i * 2 + index_offset].color = GRID_H_COLOR;

        g_GridVertex[i * 2 + 1 + index_offset].position = { x2, 0.0f, z2 };
        g_GridVertex[i * 2 + 1 + index_offset].color = GRID_H_COLOR;
    }

    for (int i = 0; i < GRID_H_LINE_COUNT; i++)
    {
        int index_offset = GRID_V_LINE_COUNT * 2;

        float x1 = -GRID_HALF_EXTENT_X + GRID_SIZE * i;
        float x2 = x1;

        float z1 = -GRID_HALF_EXTENT_Z;
        float z2 = GRID_HALF_EXTENT_Z;

        g_GridVertex[i * 2 + index_offset].position = { x1, 0.0f, z1 };
        g_GridVertex[i * 2 + index_offset].color = GRID_V_COLOR;

        g_GridVertex[i * 2 + 1 + index_offset].position = { x2, 0.0f, z2 };
        g_GridVertex[i * 2 + 1 + index_offset].color = GRID_V_COLOR;
    }
    
    // 頂点バッファ生成
    D3D11_BUFFER_DESC bd = {};
    // bd.Usage = D3D11_USAGE_DYNAMIC; // 書き換えて使えます
    bd.Usage = D3D11_USAGE_DEFAULT; // 変換行列があるため、実質動かなくていいです
    bd.ByteWidth = sizeof(Vertex3d) * NUM_VERTEX;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = g_GridVertex;

    g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);

    g_GridTexId = Texture_Load(TEXTURE_PATH.c_str());
}

void Grid_Finalize()
{
    SAFE_RELEASE(g_pVertexBuffer);
}

void Grid_Draw()
{
    Shader3D_Begin();
    Direct3D_DepthStencilStateDepthIsEnable(true);

    // 頂点バッファを描画パイプラインに設定
    UINT stride = sizeof(Vertex3d);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    // 頂点シェーダーに変換行列を設定
    // ワールド座標変換行列
    XMMATRIX mtxWorld = XMMatrixIdentity();
    Shader3D_SetWorldMatrix(mtxWorld);

    // プリミティブトポロジ設定
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    // ポリゴン描画命令発行
    // g_pContext->Draw(NUM_VERTEX, 0);
    Texture_SetTexture(g_GridTexId);
    g_pContext->Draw(NUM_VERTEX, 0);

    Direct3D_DepthStencilStateDepthIsEnable(false);
}
