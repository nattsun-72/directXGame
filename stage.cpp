/****************************************
 * @file stage.cpp
 * @brief ステージ管理の実装（MeshField統合版）
 * @author Natsume Shidara
 * @date 2026/01/13
 ****************************************/

#include "stage.h"
#include "meshfield.h"
#include "direct3d.h"
#include "texture.h"
#include "model.h"
#include "debug_renderer.h"

#include <cstdlib>
#include <cmath>
#include <algorithm>

using namespace DirectX;

//======================================
// 定数
//======================================
namespace
{
    // モデルパス
    constexpr const char* BOX_MODEL_PATH = "assets/fbx/BOX.fbx";

    // テクスチャパス
    constexpr const wchar_t* WALL_TEXTURE_PATH = L"assets/wall.png";

    // 壁の数
    constexpr int WALL_COUNT = 4;
}

//======================================
// 壁構造体
//======================================
struct Wall
{
    MODEL* pModel = nullptr;
    XMFLOAT3 position = { 0, 0, 0 };
    XMFLOAT3 scale = { 1, 1, 1 };
};

//======================================
// 内部変数
//======================================
static Wall g_Walls[WALL_COUNT];
static int g_WallTexId = -1;

//======================================
// 内部関数
//======================================
static float RandomFloat(float min, float max)
{
    float r = static_cast<float>(rand()) / RAND_MAX;
    return min + r * (max - min);
}

static void DrawWall(const Wall& wall)
{
    if (!wall.pModel) return;

    XMMATRIX mtxScale = XMMatrixScaling(wall.scale.x, wall.scale.y, wall.scale.z);
    XMMATRIX mtxTrans = XMMatrixTranslation(wall.position.x, wall.position.y, wall.position.z);
    XMMATRIX mtxWorld = mtxScale * mtxTrans;

    ModelDraw(wall.pModel, mtxWorld);
}

static void DrawWallShadow(const Wall& wall)
{
    if (!wall.pModel) return;

    XMMATRIX mtxScale = XMMatrixScaling(wall.scale.x, wall.scale.y, wall.scale.z);
    XMMATRIX mtxTrans = XMMatrixTranslation(wall.position.x, wall.position.y, wall.position.z);
    XMMATRIX mtxWorld = mtxScale * mtxTrans;

    ModelDrawShadow(wall.pModel, mtxWorld);
}

//======================================
// 初期化
//======================================
void Stage_Initialize()
{
    using namespace StageConfig;

    // MeshField初期化
    MeshField_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());

    // テクスチャ読み込み
    g_WallTexId = Texture_Load(WALL_TEXTURE_PATH);

    // 壁モデル生成（北・南・東・西）
    float wallBaseY = WALL_HEIGHT * 0.5f;

    // 北壁（+Z方向）
    g_Walls[0].pModel = ModelLoad(BOX_MODEL_PATH, 1.0f, false, WALL_TEXTURE_PATH);
    g_Walls[0].position = { 0.0f, wallBaseY, STAGE_DEPTH * 0.5f };
    g_Walls[0].scale = { STAGE_WIDTH + WALL_THICKNESS * 2, WALL_HEIGHT, WALL_THICKNESS };

    // 南壁（-Z方向）
    g_Walls[1].pModel = ModelLoad(BOX_MODEL_PATH, 1.0f, false, WALL_TEXTURE_PATH);
    g_Walls[1].position = { 0.0f, wallBaseY, -STAGE_DEPTH * 0.5f };
    g_Walls[1].scale = { STAGE_WIDTH + WALL_THICKNESS * 2, WALL_HEIGHT, WALL_THICKNESS };

    // 東壁（+X方向）
    g_Walls[2].pModel = ModelLoad(BOX_MODEL_PATH, 1.0f, false, WALL_TEXTURE_PATH);
    g_Walls[2].position = { STAGE_WIDTH * 0.5f, wallBaseY, 0.0f };
    g_Walls[2].scale = { WALL_THICKNESS, WALL_HEIGHT, STAGE_DEPTH };

    // 西壁（-X方向）
    g_Walls[3].pModel = ModelLoad(BOX_MODEL_PATH, 1.0f, false, WALL_TEXTURE_PATH);
    g_Walls[3].position = { -STAGE_WIDTH * 0.5f, wallBaseY, 0.0f };
    g_Walls[3].scale = { WALL_THICKNESS, WALL_HEIGHT, STAGE_DEPTH };
}

//======================================
// 終了処理
//======================================
void Stage_Finalize()
{
    // MeshField終了
    MeshField_Finalize();

    // 壁モデル解放
    for (int i = 0; i < WALL_COUNT; i++)
    {
        if (g_Walls[i].pModel)
        {
            ModelRelease(g_Walls[i].pModel);
            g_Walls[i].pModel = nullptr;
        }
    }
}

//======================================
// 更新
//======================================
void Stage_Update(float dt)
{
    MeshField_Update(dt);
}

//======================================
// 描画
//======================================
void Stage_Draw()
{
    // 地形（MeshField）描画
    XMMATRIX mtxWorld = XMMatrixIdentity();
    MeshField_Draw(mtxWorld);

    // 壁描画
    for (int i = 0; i < WALL_COUNT; i++)
    {
        DrawWall(g_Walls[i]);
    }
}

//======================================
// シャドウ描画
//======================================
void Stage_DrawShadow()
{
    // 地形シャドウ
    XMMATRIX mtxWorld = XMMatrixIdentity();
    MeshField_DrawShadow(mtxWorld);

    // 壁シャドウ
    for (int i = 0; i < WALL_COUNT; i++)
    {
        DrawWallShadow(g_Walls[i]);
    }
}

//======================================
// デバッグ描画
//======================================
void Stage_DrawDebug()
{
    using namespace StageConfig;

    // プレイエリアの境界を表示
    XMFLOAT4 color = { 0.0f, 1.0f, 0.0f, 1.0f };

    // 床の四隅（高さを地形に合わせる）
    float h00 = MeshField_GetHeight(PLAY_AREA_MIN_X, PLAY_AREA_MIN_Z) + 0.2f;
    float h10 = MeshField_GetHeight(PLAY_AREA_MAX_X, PLAY_AREA_MIN_Z) + 0.2f;
    float h11 = MeshField_GetHeight(PLAY_AREA_MAX_X, PLAY_AREA_MAX_Z) + 0.2f;
    float h01 = MeshField_GetHeight(PLAY_AREA_MIN_X, PLAY_AREA_MAX_Z) + 0.2f;

    XMFLOAT3 corners[4] = {
        { PLAY_AREA_MIN_X, h00, PLAY_AREA_MIN_Z },
        { PLAY_AREA_MAX_X, h10, PLAY_AREA_MIN_Z },
        { PLAY_AREA_MAX_X, h11, PLAY_AREA_MAX_Z },
        { PLAY_AREA_MIN_X, h01, PLAY_AREA_MAX_Z }
    };

    for (int i = 0; i < 4; i++)
    {
        int next = (i + 1) % 4;
        DebugRenderer::DrawLine(corners[i], corners[next], color);
    }

    // スポーン禁止エリア表示
    XMFLOAT4 safeColor = { 1.0f, 0.5f, 0.0f, 1.0f };
    constexpr int segments = 16;
    for (int i = 0; i < segments; i++)
    {
        float angle1 = (i / static_cast<float>(segments)) * 6.28318f;
        float angle2 = ((i + 1) / static_cast<float>(segments)) * 6.28318f;

        XMFLOAT3 p1 = { cosf(angle1) * PLAYER_SAFE_RADIUS, 0.5f, sinf(angle1) * PLAYER_SAFE_RADIUS };
        XMFLOAT3 p2 = { cosf(angle2) * PLAYER_SAFE_RADIUS, 0.5f, sinf(angle2) * PLAYER_SAFE_RADIUS };

        DebugRenderer::DrawLine(p1, p2, safeColor);
    }
}

//======================================
// 地上型敵スポーン位置取得
//======================================
XMFLOAT3 Stage_GetGroundSpawnPosition(const XMFLOAT3& playerPos)
{
    using namespace StageConfig;

    XMFLOAT3 spawnPos;
    int maxAttempts = 20;

    for (int i = 0; i < maxAttempts; i++)
    {
        // ランダム位置を生成
        spawnPos.x = RandomFloat(PLAY_AREA_MIN_X + SPAWN_MARGIN, PLAY_AREA_MAX_X - SPAWN_MARGIN);
        spawnPos.z = RandomFloat(PLAY_AREA_MIN_Z + SPAWN_MARGIN, PLAY_AREA_MAX_Z - SPAWN_MARGIN);

        // 地形の高さを取得
        float terrainHeight = MeshField_GetHeight(spawnPos.x, spawnPos.z);
        spawnPos.y = terrainHeight + SPAWN_HEIGHT_GROUND;

        // プレイヤーからの距離をチェック
        float dx = spawnPos.x - playerPos.x;
        float dz = spawnPos.z - playerPos.z;
        float distSq = dx * dx + dz * dz;

        if (distSq > PLAYER_SAFE_RADIUS * PLAYER_SAFE_RADIUS)
        {
            return spawnPos;
        }
    }

    // 最大試行回数を超えたら、プレイヤーから離れた位置を強制的に選択
    float angle = RandomFloat(0.0f, 6.28318f);
    spawnPos.x = playerPos.x + cosf(angle) * (PLAYER_SAFE_RADIUS + 5.0f);
    spawnPos.z = playerPos.z + sinf(angle) * (PLAYER_SAFE_RADIUS + 5.0f);

    // プレイエリア内にクランプ
    spawnPos = Stage_ClampToPlayArea(spawnPos);
    spawnPos.y = MeshField_GetHeight(spawnPos.x, spawnPos.z) + SPAWN_HEIGHT_GROUND;

    return spawnPos;
}

//======================================
// 飛行型敵スポーン位置取得
//======================================
XMFLOAT3 Stage_GetFlyingSpawnPosition(const XMFLOAT3& playerPos)
{
    using namespace StageConfig;

    XMFLOAT3 spawnPos;
    int maxAttempts = 20;

    for (int i = 0; i < maxAttempts; i++)
    {
        // ランダム位置を生成（高い位置）
        spawnPos.x = RandomFloat(PLAY_AREA_MIN_X + SPAWN_MARGIN, PLAY_AREA_MAX_X - SPAWN_MARGIN);
        spawnPos.y = RandomFloat(SPAWN_HEIGHT_FLYING, SPAWN_HEIGHT_FLYING + 5.0f);
        spawnPos.z = RandomFloat(PLAY_AREA_MIN_Z + SPAWN_MARGIN, PLAY_AREA_MAX_Z - SPAWN_MARGIN);

        // プレイヤーからの水平距離をチェック
        float dx = spawnPos.x - playerPos.x;
        float dz = spawnPos.z - playerPos.z;
        float distSq = dx * dx + dz * dz;

        if (distSq > PLAYER_SAFE_RADIUS * PLAYER_SAFE_RADIUS)
        {
            return spawnPos;
        }
    }

    // 最大試行回数を超えたら強制配置
    float angle = RandomFloat(0.0f, 6.28318f);
    spawnPos.x = playerPos.x + cosf(angle) * (PLAYER_SAFE_RADIUS + 5.0f);
    spawnPos.y = SPAWN_HEIGHT_FLYING;
    spawnPos.z = playerPos.z + sinf(angle) * (PLAYER_SAFE_RADIUS + 5.0f);

    spawnPos = Stage_ClampToPlayArea(spawnPos);
    spawnPos.y = SPAWN_HEIGHT_FLYING;  // 高度は維持

    return spawnPos;
}

//======================================
// プレイエリア内判定
//======================================
bool Stage_IsInsidePlayArea(const XMFLOAT3& pos)
{
    using namespace StageConfig;

    return pos.x >= PLAY_AREA_MIN_X && pos.x <= PLAY_AREA_MAX_X &&
        pos.z >= PLAY_AREA_MIN_Z && pos.z <= PLAY_AREA_MAX_Z;
}

//======================================
// プレイエリア内にクランプ
//======================================
XMFLOAT3 Stage_ClampToPlayArea(const XMFLOAT3& pos)
{
    using namespace StageConfig;

    XMFLOAT3 result = pos;
    result.x = std::clamp(result.x, PLAY_AREA_MIN_X + 0.5f, PLAY_AREA_MAX_X - 0.5f);
    result.z = std::clamp(result.z, PLAY_AREA_MIN_Z + 0.5f, PLAY_AREA_MAX_Z - 0.5f);

    return result;
}

//======================================
// 地形高さ取得
//======================================
float Stage_GetTerrainHeight(float x, float z)
{
    return MeshField_GetHeight(x, z);
}