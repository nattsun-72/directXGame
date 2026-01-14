/****************************************
 * @file meshfield.cpp
 * @brief メッシュフィールド（地形）生成・描画の実装
 * @author Natsume Shidara
 * @date 2025/12/10
 * @update 2026/01/13 - 地形隆起機能追加
 ****************************************/

#include "meshfield.h"
#include <DirectXMath.h>
#include <cmath>
#include <algorithm>
#include "direct3d.h"
#include "texture.h"
#include "shader3d.h"
#include "shader_shadow_map.h"

using namespace DirectX;

//======================================
// 定数・設定
//======================================
namespace
{
    using namespace TerrainConfig;

    // 頂点数
    constexpr int MESH_H_VERTEX_COUNT = MESH_H_COUNT + 1;
    constexpr int MESH_V_VERTEX_COUNT = MESH_V_COUNT + 1;

    // 頂点/インデックス数
    constexpr int NUM_VERTEX = MESH_H_VERTEX_COUNT * MESH_V_VERTEX_COUNT;
    constexpr int NUM_INDEX = 3 * 2 * MESH_H_COUNT * MESH_V_COUNT;

    // メッシュ全体の半径
    constexpr float MESH_HALF_EXTENT_X = TERRAIN_WIDTH * 0.5f;
    constexpr float MESH_HALF_EXTENT_Z = TERRAIN_DEPTH * 0.5f;

    // テクスチャパス
    const wchar_t* TEXTURE_PATH = L"assets/floor.png";
}

//======================================
// 頂点構造体
//======================================
struct Vertex3d
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT4 color;
    XMFLOAT2 uv;
};

//======================================
// グローバル変数
//======================================
static ID3D11Buffer* g_pVertexBuffer = nullptr;
static ID3D11Buffer* g_pIndexBuffer = nullptr;
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;
static int g_MeshTexId = -1;

// 頂点データとインデックスデータ
static Vertex3d g_MeshVertex[NUM_VERTEX];
static unsigned short g_MeshIndex[NUM_INDEX];

// 高さマップ（GetHeight用にキャッシュ）
static float g_HeightMap[MESH_H_VERTEX_COUNT][MESH_V_VERTEX_COUNT];

//======================================
// ノイズ関数（簡易パーリンノイズ風）
//======================================
namespace Noise
{
    // 簡易ハッシュ関数
    static float Hash(float x, float z)
    {
        int ix = static_cast<int>(floorf(x)) * 73856093;
        int iz = static_cast<int>(floorf(z)) * 19349663;
        int hash = (ix ^ iz) & 0x7FFFFFFF;
        return static_cast<float>(hash % 10000) / 10000.0f;
    }

    // 補間関数（スムースステップ）
    static float SmoothStep(float t)
    {
        return t * t * (3.0f - 2.0f * t);
    }

    // バリューノイズ
    static float ValueNoise(float x, float z)
    {
        float fx = floorf(x);
        float fz = floorf(z);
        float tx = x - fx;
        float tz = z - fz;

        // 4隅の値
        float v00 = Hash(fx, fz);
        float v10 = Hash(fx + 1, fz);
        float v01 = Hash(fx, fz + 1);
        float v11 = Hash(fx + 1, fz + 1);

        // 補間
        float sx = SmoothStep(tx);
        float sz = SmoothStep(tz);

        float v0 = v00 + sx * (v10 - v00);
        float v1 = v01 + sx * (v11 - v01);

        return v0 + sz * (v1 - v0);
    }

    // フラクタルブラウン運動（複数オクターブのノイズ）
    static float FBM(float x, float z, int octaves, float persistence, float lacunarity)
    {
        float total = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; i++)
        {
            total += ValueNoise(x * frequency, z * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return total / maxValue;
    }
}

//======================================
// 地形高さ生成
//======================================
static float GenerateTerrainHeight(float x, float z)
{
    using namespace TerrainConfig;

    // 基本のノイズ地形
    float noiseScale = 0.08f;
    float height = Noise::FBM(x * noiseScale, z * noiseScale, 4, 0.5f, 2.0f);

    // 0～1 を MIN_HEIGHT～MAX_HEIGHT に変換
    height = MIN_HEIGHT + height * (MAX_HEIGHT - MIN_HEIGHT);

    // 丘の追加（正弦波を組み合わせ）
    float hill1 = sinf(x * 0.15f) * cosf(z * 0.12f) * 0.5f;
    float hill2 = sinf(x * 0.08f + 1.5f) * sinf(z * 0.1f + 0.8f) * 0.3f;
    height += hill1 + hill2;

    // 中央付近を少し低くする（アリーナ感）
    float distFromCenter = sqrtf(x * x + z * z);
    float centerFlatten = 1.0f - expf(-distFromCenter * distFromCenter * 0.002f);
    height *= centerFlatten;

    // 端を平坦にする
    float edgeX = std::fmaxf(0.0f, (fabsf(x) - (MESH_HALF_EXTENT_X - EDGE_FLATTEN_MARGIN)) / EDGE_FLATTEN_MARGIN);
    float edgeZ = std::fmaxf(0.0f, (fabsf(z) - (MESH_HALF_EXTENT_Z - EDGE_FLATTEN_MARGIN)) / EDGE_FLATTEN_MARGIN);
    float edgeFactor = std::fmaxf(edgeX, edgeZ);
    edgeFactor = std::fminf(1.0f, edgeFactor);
    height *= (1.0f - edgeFactor);

    return height;
}

//======================================
// 法線計算
//======================================
static XMFLOAT3 CalculateNormal(int x, int z)
{
    // 隣接頂点との高さ差から法線を計算
    float hL = (x > 0) ? g_HeightMap[x - 1][z] : g_HeightMap[x][z];
    float hR = (x < MESH_H_VERTEX_COUNT - 1) ? g_HeightMap[x + 1][z] : g_HeightMap[x][z];
    float hD = (z > 0) ? g_HeightMap[x][z - 1] : g_HeightMap[x][z];
    float hU = (z < MESH_V_VERTEX_COUNT - 1) ? g_HeightMap[x][z + 1] : g_HeightMap[x][z];

    XMVECTOR normal = XMVectorSet(hL - hR, 2.0f * MESH_SIZE, hD - hU, 0.0f);
    normal = XMVector3Normalize(normal);

    XMFLOAT3 result;
    XMStoreFloat3(&result, normal);
    return result;
}

//======================================
// 頂点色（高さに応じて変化）
//======================================
static XMFLOAT4 GetHeightColor(float height)
{
    using namespace TerrainConfig;

    // 正規化（0～1）
    float t = (height - MIN_HEIGHT) / (MAX_HEIGHT - MIN_HEIGHT);
    t = std::clamp(t, 0.0f, 1.0f);

    // 低い部分：暗い青灰色、高い部分：明るい灰色
    XMFLOAT4 lowColor = { 0.35f, 0.38f, 0.45f, 1.0f };
    XMFLOAT4 highColor = { 0.55f, 0.55f, 0.6f, 1.0f };

    return {
        lowColor.x + t * (highColor.x - lowColor.x),
        lowColor.y + t * (highColor.y - lowColor.y),
        lowColor.z + t * (highColor.z - lowColor.z),
        1.0f
    };
}

//======================================
// 頂点生成
//======================================
static void CreateVertices()
{
    // まず高さマップを生成
    for (int z = 0; z < MESH_V_VERTEX_COUNT; z++)
    {
        for (int x = 0; x < MESH_H_VERTEX_COUNT; x++)
        {
            float posX = -MESH_HALF_EXTENT_X + x * MESH_SIZE;
            float posZ = -MESH_HALF_EXTENT_Z + z * MESH_SIZE;
            g_HeightMap[x][z] = GenerateTerrainHeight(posX, posZ);
        }
    }

    // 頂点データを生成
    int index = 0;
    for (int z = 0; z < MESH_V_VERTEX_COUNT; z++)
    {
        for (int x = 0; x < MESH_H_VERTEX_COUNT; x++)
        {
            float posX = -MESH_HALF_EXTENT_X + x * MESH_SIZE;
            float posZ = -MESH_HALF_EXTENT_Z + z * MESH_SIZE;
            float posY = g_HeightMap[x][z];

            g_MeshVertex[index].position = { posX, posY, posZ };
            g_MeshVertex[index].normal = CalculateNormal(x, z);
            g_MeshVertex[index].color = GetHeightColor(posY);
            g_MeshVertex[index].uv = { x * 0.5f, z * 0.5f };  // UVタイリング

            index++;
        }
    }
}

//======================================
// インデックス生成
//======================================
static void CreateIndices()
{
    int idx = 0;
    for (int z = 0; z < MESH_V_COUNT; z++)
    {
        for (int x = 0; x < MESH_H_COUNT; x++)
        {
            int v0 = z * MESH_H_VERTEX_COUNT + x;
            int v1 = v0 + 1;
            int v2 = v0 + MESH_H_VERTEX_COUNT;
            int v3 = v2 + 1;

            // 三角形1
            g_MeshIndex[idx++] = static_cast<unsigned short>(v0);
            g_MeshIndex[idx++] = static_cast<unsigned short>(v2);
            g_MeshIndex[idx++] = static_cast<unsigned short>(v3);

            // 三角形2
            g_MeshIndex[idx++] = static_cast<unsigned short>(v3);
            g_MeshIndex[idx++] = static_cast<unsigned short>(v1);
            g_MeshIndex[idx++] = static_cast<unsigned short>(v0);
        }
    }
}

//======================================
// 初期化
//======================================
void MeshField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    if (!pDevice || !pContext) return;

    g_pDevice = pDevice;
    g_pContext = pContext;

    // 頂点データ生成
    CreateVertices();

    // 頂点バッファ作成
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex3d) * NUM_VERTEX;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = g_MeshVertex;

    g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);

    // インデックスデータ生成
    CreateIndices();

    // インデックスバッファ作成
    bd.ByteWidth = sizeof(unsigned short) * NUM_INDEX;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    sd.pSysMem = g_MeshIndex;

    g_pDevice->CreateBuffer(&bd, &sd, &g_pIndexBuffer);

    // テクスチャ読み込み
    g_MeshTexId = Texture_Load(TEXTURE_PATH);
}

//======================================
// 終了処理
//======================================
void MeshField_Finalize()
{
    if (g_pVertexBuffer)
    {
        g_pVertexBuffer->Release();
        g_pVertexBuffer = nullptr;
    }
    if (g_pIndexBuffer)
    {
        g_pIndexBuffer->Release();
        g_pIndexBuffer = nullptr;
    }
}

//======================================
// 更新
//======================================
void MeshField_Update(double elapsed_time)
{
    (void)elapsed_time;
}

//======================================
// 描画
//======================================
void MeshField_Draw(const XMMATRIX& mtxWorld)
{
    if (!g_pContext) return;

    Shader3D_Begin();
    Direct3D_DepthStencilStateDepthIsEnable(true);

    Texture_SetTexture(g_MeshTexId);

    UINT stride = sizeof(Vertex3d);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Shader3D_SetWorldMatrix(mtxWorld);
    Shader3D_SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    g_pContext->DrawIndexed(NUM_INDEX, 0, 0);
}

//======================================
// シャドウ描画
//======================================
void MeshField_DrawShadow(const XMMATRIX& mtxWorld)
{
    if (!g_pContext) return;

    ShaderShadowMap_SetWorldMatrix(mtxWorld);

    UINT stride = sizeof(Vertex3d);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    g_pContext->DrawIndexed(NUM_INDEX, 0, 0);
}

//======================================
// 指定座標の高さを取得
//======================================
float MeshField_GetHeight(float x, float z)
{
    // 座標をグリッドインデックスに変換
    float localX = x + MESH_HALF_EXTENT_X;
    float localZ = z + MESH_HALF_EXTENT_Z;

    int gridX = static_cast<int>(localX / MESH_SIZE);
    int gridZ = static_cast<int>(localZ / MESH_SIZE);

    // 範囲外チェック
    if (gridX < 0 || gridX >= MESH_H_COUNT || gridZ < 0 || gridZ >= MESH_V_COUNT)
    {
        return 0.0f;
    }

    // グリッド内の相対位置（0～1）
    float tx = (localX - gridX * MESH_SIZE) / MESH_SIZE;
    float tz = (localZ - gridZ * MESH_SIZE) / MESH_SIZE;

    // 4頂点の高さ
    float h00 = g_HeightMap[gridX][gridZ];
    float h10 = g_HeightMap[gridX + 1][gridZ];
    float h01 = g_HeightMap[gridX][gridZ + 1];
    float h11 = g_HeightMap[gridX + 1][gridZ + 1];

    // バイリニア補間
    float h0 = h00 + tx * (h10 - h00);
    float h1 = h01 + tx * (h11 - h01);

    return h0 + tz * (h1 - h0);
}

//======================================
// 指定座標の法線を取得
//======================================
XMFLOAT3 MeshField_GetNormal(float x, float z)
{
    // 近傍4点から法線を計算
    float delta = MESH_SIZE;
    float hL = MeshField_GetHeight(x - delta, z);
    float hR = MeshField_GetHeight(x + delta, z);
    float hD = MeshField_GetHeight(x, z - delta);
    float hU = MeshField_GetHeight(x, z + delta);

    XMVECTOR normal = XMVectorSet(hL - hR, 2.0f * delta, hD - hU, 0.0f);
    normal = XMVector3Normalize(normal);

    XMFLOAT3 result;
    XMStoreFloat3(&result, normal);
    return result;
}

//======================================
// 地形サイズ取得
//======================================
float MeshField_GetWidth()
{
    return TerrainConfig::TERRAIN_WIDTH;
}

float MeshField_GetDepth()
{
    return TerrainConfig::TERRAIN_DEPTH;
}