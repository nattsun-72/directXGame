/****************************************
 * @file game.cpp
 * @brief ゲーム管理クラス（完全統合版）
 * @detail
 * - Stageによるアリーナ（床＋4方向の壁）
 * - ランダム敵スポーンシステム
 * - シャドウマップ生成と描画
 * - Blade統合（一人称視点）
 * - ポストプロセス（ブラー効果）
 * @author Natsume Shidara
 * @update 2026/01/13 - ステージ＆ランダムスポーン追加
 * @update 2026/01/13 - サウンド対応・デバッグ機能分離
 * @update 2026/02/04 - パーティクルシステム追加
 ****************************************/

#include "game.h"

 //--------------------------------------
 // システム・共通ヘッダ
 //--------------------------------------
#include "direct3d.h"
#include "camera.h"
#include "player_camera.h"
#include "light.h"
#include "texture.h"
#include "key_logger.h"
#include "pad_logger.h"
#include "input_manager.h"
#include "ray.h"
#include "collision.h"

#include "shader_shadow_map.h" 
#include "light_camera.h"

//--------------------------------------
// オブジェクト・マネージャーヘッダ
//--------------------------------------
#include "stage.h"
#include "prop_manager.h"
#include "player.h"
#include "enemy.h"
#include "enemy_bullet.h"
#include "bullet.h"
#include "billboard.h"
#include "bullet_hit_effect.h"
#include "trail.h"
#include "skybox.h"
#include "sprite_anim.h"
#include "blade.h"
#include "post_process.h"
#include "hp_gauge.h"
#include "score.h"
#include "game_timer.h"
#include "combo.h"
#include "scene.h"
#include "fade.h"

#include "particle_test.h"

#include "sound_manager.h"

#ifdef _DEBUG
#include "debug_renderer.h"
#endif

#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace DirectX;

//--------------------------------------
// 定数
//--------------------------------------
namespace
{
    constexpr float GAME_TIME_LIMIT = 180.0f;

    constexpr float SCORE_MARGIN_X = 20.0f;
    constexpr float SCORE_MARGIN_Y = 60.0f;
    constexpr int SCORE_DIGITS = 7;

    constexpr float ENEMY_SPAWN_INTERVAL_BASE = 4.0f;
    constexpr float ENEMY_SPAWN_INTERVAL_MIN = 1.5f;
    constexpr float SPAWN_RATE_INCREASE = 0.015f;
    constexpr int MAX_ENEMIES = 15;
    constexpr float FLYING_SPAWN_CHANCE = 0.35f;
    constexpr int INITIAL_ENEMY_COUNT = 3;

    constexpr XMFLOAT3 PLAYER_CAMERA_OFFSET = { 0.0f, 1.6f, 0.0f };
    constexpr XMFLOAT3 PLAYER_START_POSITION = { 0.0f, 1.0f, 0.0f };
    constexpr XMFLOAT3 PLAYER_START_FRONT = { 0.0f, 0.0f, 1.0f };

    constexpr XMFLOAT3 LIGHT_OFFSET = { 25.0f, 50.0f, -25.0f };
    constexpr XMFLOAT4 LIGHT_DIRECTION = { -0.3f, -0.75f, 0.3f, 0.0f };
    constexpr XMFLOAT4 LIGHT_COLOR = { 1.1f, 1.08f, 1.0f, 1.0f };
    constexpr XMFLOAT3 AMBIENT_COLOR = { 0.5f, 0.5f, 0.55f };

#ifdef _DEBUG
    constexpr XMFLOAT3 DEBUG_CAMERA_POSITION = { 0.0f, 20.0f, -40.0f };
    constexpr XMFLOAT3 DEBUG_CAMERA_FRONT = { 0.0f, -0.3f, 1.0f };
    constexpr XMFLOAT3 DEBUG_CAMERA_UP = { 0.0f, 1.0f, 0.0f };
#endif
}

//--------------------------------------
// グローバル変数
//--------------------------------------
static int g_BillboardTexId = -1;
static int g_BillboardAnimId = -1;
static int g_BillboardPatternId = -1;

static float g_EnemySpawnTimer = 0.0f;
static float g_GameElapsedTime = 0.0f;

static bool g_IsGameOver = false;
static bool g_IsTransitioning = false;

static NormalEmitter* g_pTestEmitter = nullptr;

#ifdef _DEBUG
static bool g_IsDebugCamera = false;
static bool g_IsDebugDraw = false;
#endif

//--------------------------------------
// 内部関数プロトタイプ
//--------------------------------------
static void UpdateSystems(float dt);
static void UpdateObjects(float dt);
static void UpdateEnemySpawn(float dt);
static void UpdateLightCamera();
static void UpdateCollisions();
static void CheckGameEnd();

static void DrawScene();

static float GetCurrentSpawnInterval();
static ENEMY_TYPE GetRandomEnemyType();

#ifdef _DEBUG
static void ProcessDebugInput();
static void DrawDebugOverlay(const XMFLOAT4X4& mtxView, const XMFLOAT4X4& mtxProj);
#endif

//======================================
// ゲーム初期化
//======================================
void Game_Initialize()
{
    srand(static_cast<unsigned int>(time(nullptr)));

    InputManager_Initialize();
    SpriteAnim_Initialize();

    Stage_Initialize();
    PropManager_Initialize();

    // カメラ初期化（定数バッファ作成のため、リリースでも必要）
    Camera_Initialize();

#ifdef _DEBUG
    // デバッグカメラの初期位置設定
    Camera_Initialize(DEBUG_CAMERA_POSITION, DEBUG_CAMERA_FRONT, DEBUG_CAMERA_UP);
#endif

    PLCamera_Initialize(PLAYER_CAMERA_OFFSET);

    Player_Initialize(PLAYER_START_POSITION, PLAYER_START_FRONT);
    Enemy_Initialize();
    EnemyBullet_Initialize();
    Bullet_Initialize();
    Billboard_Initialize();
    BulletHItEffect_Initialize();
    Trail_Initialize();
    Skybox_Initialize();
    Blade_Initialize();

    PostProcess_Initialize();

    HpGauge_Initialize();

    float screenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    float scoreX = screenWidth - SCORE_MARGIN_X - (22.0f * SCORE_DIGITS);
    Score_Initialize(scoreX, SCORE_MARGIN_Y, SCORE_DIGITS);

    GameTimer_Initialize(GAME_TIME_LIMIT);
    Combo_Initialize();

    g_BillboardTexId = Texture_Load(L"assets/runningman001.png");
    g_BillboardPatternId = SpriteAnim_RegisterPattern(g_BillboardTexId, 10, 5, 0.1, { 140, 200 }, { 0, 0 });
    g_BillboardAnimId = SpriteAnim_CreatePlayer(g_BillboardPatternId);

#ifdef _DEBUG
    DebugRenderer::Initialize();
#endif

    g_EnemySpawnTimer = 0.0f;
    g_GameElapsedTime = 0.0f;
    g_IsGameOver = false;
    g_IsTransitioning = false;

#ifdef _DEBUG
    g_IsDebugCamera = false;
    g_IsDebugDraw = false;
#endif

    XMFLOAT3 playerPos = Player_GetPosition();
    for (int i = 0; i < INITIAL_ENEMY_COUNT; i++)
    {
        XMFLOAT3 spawnPos = Stage_GetGroundSpawnPosition(playerPos);
        Enemy_Create(ENEMY_TYPE_GROUND, spawnPos);
    }

    // BGM再生
    SoundManager_PlayBGM(SOUND_BGM_GAME);

    Fade_Start(1.0, false);

    // パーティクルエミッター初期化
    XMVECTOR emitterPos = XMVectorSet(0.0f, 2.0f, 5.0f, 0.0f);
    g_pTestEmitter = new NormalEmitter(100, emitterPos, 10.0, true);
}

//======================================
// ゲーム終了
//======================================
void Game_Finalize()
{
    // パーティクル解放
    if (g_pTestEmitter)
    {
        delete g_pTestEmitter;
        g_pTestEmitter = nullptr;
    }

#ifdef _DEBUG
    DebugRenderer::Finalize();
#endif

    Combo_Finalize();
    GameTimer_Finalize();
    Score_Finalize();
    HpGauge_Finalize();
    PostProcess_Finalize();
    Blade_Finalize();
    PropManager_Finalize();
    Stage_Finalize();

    Skybox_Finalize();
    Trail_Finalize();
    BulletHItEffect_Finalize();
    Billboard_Finalize();
    Bullet_Finalize();
    EnemyBullet_Finalize();
    Enemy_Finalize();
    Player_Finalize();

    // カメラ終了（リリースでも必要）
    Camera_Finalize();
    PLCamera_Finalize();

    SpriteAnim_Finalize();
    InputManager_Finalize();
}

//======================================
// ゲーム更新
//======================================
void Game_Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);

    InputManager_Update();

    CheckGameEnd();

    if (g_IsTransitioning)
    {
        if (Fade_GetState() == FADE_STATE::FINISHED_OUT)
        {
            Scene_Change(Scene::RESULT);
        }
        return;
    }

    if (g_IsGameOver)
    {
        HpGauge_Update(dt);
        Score_Update(dt);
        return;
    }

    g_GameElapsedTime += dt;

    UpdateSystems(dt);
    UpdateObjects(dt);
    UpdateEnemySpawn(dt);
    UpdateLightCamera();
    UpdateCollisions();

#ifdef _DEBUG
    ProcessDebugInput();
#endif

    PostProcess_Update(dt);

    HpGauge_Update(dt);
    Score_Update(dt);
    GameTimer_Update(dt);
    Combo_Update(dt);
}

//--------------------------------------
// システム更新
//--------------------------------------
static void UpdateSystems(float dt)
{
    SpriteAnim_Update(dt);
    Blade_Update(dt);

#ifdef _DEBUG
    if (g_IsDebugCamera)
    {
        Camera_Update(dt);
    }
    else
    {
        PLCamera_Update(dt);
        Player_Update(dt);
    }
#else
    PLCamera_Update(dt);
    Player_Update(dt);
#endif
}

//--------------------------------------
// オブジェクト更新
//--------------------------------------
static void UpdateObjects(float dt)
{
    Stage_Update(dt);
    PropManager_Update(dt);

    Enemy_ProcessSliceResults();

    Enemy_Update(dt);
    EnemyBullet_Update(dt);
    Light_Update(dt);
    Bullet_Update(dt);
    BulletHItEffect_Update(dt);
    Trail_Update(dt);

    // パーティクル更新
    if (g_pTestEmitter)
    {
        g_pTestEmitter->Update(dt);
    }
}

//--------------------------------------
// 現在のスポーン間隔を取得
//--------------------------------------
static float GetCurrentSpawnInterval()
{
    float interval = ENEMY_SPAWN_INTERVAL_BASE - g_GameElapsedTime * SPAWN_RATE_INCREASE;
    return std::max(interval, ENEMY_SPAWN_INTERVAL_MIN);
}

//--------------------------------------
// ランダムな敵タイプを取得
//--------------------------------------
static ENEMY_TYPE GetRandomEnemyType()
{
    float r = static_cast<float>(rand()) / RAND_MAX;

    if (r < FLYING_SPAWN_CHANCE)
    {
        return ENEMY_TYPE_FLYING;
    }
    return ENEMY_TYPE_GROUND;
}

//--------------------------------------
// 敵スポーン処理
//--------------------------------------
static void UpdateEnemySpawn(float dt)
{
    g_EnemySpawnTimer += dt;

    float spawnInterval = GetCurrentSpawnInterval();

    if (g_EnemySpawnTimer >= spawnInterval)
    {
        g_EnemySpawnTimer = 0.0f;

        if (Enemy_GetAliveCount() >= MAX_ENEMIES)
        {
            return;
        }

        XMFLOAT3 playerPos = Player_GetPosition();
        ENEMY_TYPE enemyType = GetRandomEnemyType();

        XMFLOAT3 spawnPos;
        if (enemyType == ENEMY_TYPE_FLYING)
        {
            spawnPos = Stage_GetFlyingSpawnPosition(playerPos);
        }
        else
        {
            spawnPos = Stage_GetGroundSpawnPosition(playerPos);
        }

        Enemy_Create(enemyType, spawnPos);
    }
}

//--------------------------------------
// ライトカメラ更新
//--------------------------------------
static void UpdateLightCamera()
{
    XMFLOAT3 playerPos = Player_GetPosition();

    XMFLOAT3 lightPos = {
        playerPos.x + LIGHT_OFFSET.x,
        LIGHT_OFFSET.y,
        playerPos.z + LIGHT_OFFSET.z
    };

    LightCamera_SetPosition(lightPos);
    LightCamera_SetFront({ LIGHT_DIRECTION.x, LIGHT_DIRECTION.y, LIGHT_DIRECTION.z });
    LightCamera_SetRange(50.0f, 50.0f, 1.0f, 200.0f);
}

//--------------------------------------
// 衝突判定処理
//--------------------------------------
static void UpdateCollisions()
{
    for (int i = Bullet_GetBulletCount() - 1; i >= 0; i--)
    {
        XMFLOAT3 bulletPos = Bullet_GetPosition(i);
        if (!Stage_IsInsidePlayArea(bulletPos))
        {
            BulletHItEffect_Create(bulletPos);
            Bullet_Destory(i);
        }
    }
}

//--------------------------------------
// ゲーム終了判定
//--------------------------------------
static void CheckGameEnd()
{
    if (g_IsTransitioning) return;

    if (GameTimer_IsTimeUp())
    {
        g_IsGameOver = true;
        g_IsTransitioning = true;
        Score_SaveFinal();
        Fade_Start(1.5, true);
        return;
    }

    if (Player_GetHP() <= 0)
    {
        g_IsGameOver = true;
        g_IsTransitioning = true;
        Score_SaveFinal();
        Fade_Start(2.0, true);
        return;
    }
}

#ifdef _DEBUG
//--------------------------------------
// デバッグ入力処理
//--------------------------------------
static void ProcessDebugInput()
{
    if (KeyLogger_IsTrigger(KK_L))
    {
        g_IsDebugCamera = !g_IsDebugCamera;
    }

    if (KeyLogger_IsTrigger(KK_O))
    {
        g_IsDebugDraw = !g_IsDebugDraw;
    }

    if (KeyLogger_IsTrigger(KK_P))
    {
        XMFLOAT3 playerPos = Player_GetPosition();
        XMFLOAT3 spawnPos = Stage_GetGroundSpawnPosition(playerPos);
        Enemy_Create(ENEMY_TYPE_GROUND, spawnPos);
    }

    if (KeyLogger_IsTrigger(KK_U))
    {
        XMFLOAT3 playerPos = Player_GetPosition();
        XMFLOAT3 spawnPos = Stage_GetFlyingSpawnPosition(playerPos);
        Enemy_Create(ENEMY_TYPE_FLYING, spawnPos);
    }
}
#endif

//======================================
// シャドウマップ描画
//======================================
void Game_DrawShadow()
{
    Stage_DrawShadow();
    PropManager_DrawShadow();
    Player_DrawShadow();
    Enemy_DrawShadow();
    Blade_DrawShadow();
}

//======================================
// ゲーム描画
//======================================
void Game_Draw()
{
    PostProcess_BeginScene();

    DrawScene();

    PostProcess_EndScene();

    Direct3D_DepthStencilStateDepthIsEnable(false);
    Direct3D_SetBlendState(BlendMode::Alpha);

    HpGauge_Draw();
    Score_Draw();
    GameTimer_Draw();
    Combo_Draw();

    Direct3D_DepthStencilStateDepthIsEnable(true);

#ifdef _DEBUG
    XMFLOAT4X4 mtxView = g_IsDebugCamera ? Camera_GetViewMatrix() : PLCamera_GetViewMatrix();
    XMFLOAT4X4 mtxProj = g_IsDebugCamera ? Camera_GetPerspectiveMatrix() : PLCamera_GetPerspectiveMatrix();
    DrawDebugOverlay(mtxView, mtxProj);
#endif
}

//--------------------------------------
// シーン描画
//--------------------------------------
static void DrawScene()
{
    Direct3D_DepthStencilStateDepthIsEnable(true);

#ifdef _DEBUG
    XMFLOAT4X4 mtxView = g_IsDebugCamera ? Camera_GetViewMatrix() : PLCamera_GetViewMatrix();
    XMFLOAT4X4 mtxProj = g_IsDebugCamera ? Camera_GetPerspectiveMatrix() : PLCamera_GetPerspectiveMatrix();
#else
    XMFLOAT4X4 mtxView = PLCamera_GetViewMatrix();
    XMFLOAT4X4 mtxProj = PLCamera_GetPerspectiveMatrix();
#endif

    Camera_SetMatrix(XMLoadFloat4x4(&mtxView), XMLoadFloat4x4(&mtxProj));
    Billboard_SetViewMatrix(mtxView);

    Light_SetDirectional_World(LIGHT_DIRECTION, LIGHT_COLOR);
    Light_SetAmbient(AMBIENT_COLOR);

    Direct3D_DepthStencilStateDepthIsEnable(false);
    Skybox_SetPosition(Player_GetPosition());
    Skybox_Draw();
    Direct3D_DepthStencilStateDepthIsEnable(true);

    Stage_Draw();
    PropManager_Draw();
    Enemy_Draw();
    EnemyBullet_Draw();
    Bullet_Draw();

    Blade_Draw();

    BulletHItEffect_Draw();
    Trail_Draw();

    // パーティクル描画
    if (g_pTestEmitter)
    {
        g_pTestEmitter->Draw();
    }
}

#ifdef _DEBUG
//--------------------------------------
// デバッグオーバーレイ描画
//--------------------------------------
static void DrawDebugOverlay(const XMFLOAT4X4& mtxView, const XMFLOAT4X4& mtxProj)
{
    if (g_IsDebugCamera)
    {
        Camera_DebugDraw();
    }

    if (g_IsDebugDraw)
    {
        Stage_DrawDebug();
        PropManager_DrawDebug();
        Player_DrawDebug();
        Enemy_DrawDebug();
        EnemyBullet_DrawDebug();
        Blade_DebugDraw();

        DebugRenderer::Render(mtxView, mtxProj);
    }
}
#endif