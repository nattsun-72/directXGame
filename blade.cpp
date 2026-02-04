/****************************************
 * @file blade.cpp
 * @brief ブレード制御（一人称視点）
 * @author Natsume Shidara
 * @date 2026/01/07
 * @update 2026/01/10 - リファクタリング
 * @update 2026/01/13 - 切断法線計算の修正
 * @update 2026/01/13 - サウンド対応
 * @update 2026/02/03 - スタイリッシュ斬撃アニメーション
 * @update 2026/02/03 - 横薙ぎ復旧・速度調整
 * @update 2026/02/03 - 斬撃フェーズ中の常時切断チェック
 ****************************************/

#include "blade.h"
#include "model.h"
#include "player_camera.h"
#include "input_manager.h"
#include "ray.h"
#include "prop_manager.h"
#include "enemy.h"
#include "key_logger.h"
#include "trail.h"
#include "debug_renderer.h"
#include "shader_shadow_map.h"
#include "sound_manager.h"
#include <cmath>
#include <random>

using namespace DirectX;

//======================================
// 切断パラメータ構造体
//======================================
struct SliceParams
{
    XMFLOAT3 rayDirection;
    float rayLength;
    float planeSpread;
};

//======================================
// 斬撃パターン構造体
//======================================
struct SlashPattern
{
    // 開始姿勢
    XMFLOAT3 startRotation;
    XMFLOAT3 startOffset;

    // 終了姿勢
    XMFLOAT3 endRotation;
    XMFLOAT3 endOffset;

    // タイミング
    float windupRatio;      // 振りかぶり時間の割合
    float strikeRatio;      // 斬撃時間の割合（残りはリカバリー）
    float duration;         // 全体の長さ

    // 切断方向（カメラ座標系）
    XMFLOAT3 sliceDirection;

    // サウンド
    SoundID soundId;
};

//======================================
// 定数
//======================================
namespace
{
    constexpr const char* MODEL_PATH = "assets/fbx/LongSword/LongSword.fbx";
    constexpr float MODEL_SCALE = 0.85f;
    const XMFLOAT3 MODEL_ROTATION_OFFSET = { 1.24f, 9.71f, 1.98f };
    constexpr XMFLOAT3 DEFAULT_OFFSET = { 0.3f, -0.3f, 0.5f };

    constexpr float IDLE_SWAY_SPEED = 2.0f;
    constexpr float IDLE_SWAY_AMOUNT = 0.01f;

    // 横薙ぎ専用パラメータ
    constexpr float HORIZONTAL_SLASH_DURATION = 0.35f;
    constexpr float HORIZONTAL_ROT_X = 0.0f;
    constexpr float HORIZONTAL_ROT_Z = 3.0f;
    constexpr float HORIZONTAL_ROT_Y_START = 8.0f;
    constexpr float HORIZONTAL_ROT_Y_END = 11.0f;

    constexpr float SLICE_ROTATION_SENSITIVITY = 0.005f;
    constexpr float SLICE_SWING_THRESHOLD = 2.0f;

    const SliceParams DEFAULT_SLICE = {
        { 0.0f, 0.0f, -1.0f },
        7.5f,
        0.4f
    };

    const SliceParams HORIZONTAL_SLICE = {
        { 0.0f, 0.0f, -1.0f },
        7.5f,
        0.4f
    };

    //======================================
    // 斬撃パターン定義
    //======================================

    // パターン1: 右上→左下（袈裟斬り）
    const SlashPattern SLASH_KESA = {
        .startRotation = { -0.6f, -0.4f, -0.8f },
        .startOffset = { 0.15f, 0.1f, 0.0f },
        .endRotation = { 0.8f, 0.5f, 1.2f },
        .endOffset = { -0.1f, -0.15f, 0.0f },
        .windupRatio = 0.2f,
        .strikeRatio = 0.35f,
        .duration = 0.45f,
        .sliceDirection = { -0.7f, -0.7f, 0.0f },
        .soundId = SOUND_SE_SLASH_HEAVY
    };

    // パターン2: 左上→右下（逆袈裟）
    const SlashPattern SLASH_REVERSE_KESA = {
        .startRotation = { -0.5f, 0.5f, 0.9f },
        .startOffset = { -0.1f, 0.12f, 0.0f },
        .endRotation = { 0.7f, -0.4f, -1.0f },
        .endOffset = { 0.15f, -0.12f, 0.0f },
        .windupRatio = 0.2f,
        .strikeRatio = 0.35f,
        .duration = 0.45f,
        .sliceDirection = { 0.7f, -0.7f, 0.0f },
        .soundId = SOUND_SE_SLASH_HEAVY
    };

    // パターン3: 右横薙ぎ
    const SlashPattern SLASH_RIGHT_SWEEP = {
        .startRotation = { 0.0f, -0.6f, -1.2f },
        .startOffset = { 0.2f, 0.0f, 0.0f },
        .endRotation = { 0.1f, 0.7f, 0.8f },
        .endOffset = { -0.15f, -0.05f, 0.0f },
        .windupRatio = 0.15f,
        .strikeRatio = 0.4f,
        .duration = 0.38f,
        .sliceDirection = { -1.0f, 0.0f, 0.0f },
        .soundId = SOUND_SE_SLASH
    };

    // パターン4: 左横薙ぎ
    const SlashPattern SLASH_LEFT_SWEEP = {
        .startRotation = { 0.0f, 0.6f, 1.0f },
        .startOffset = { -0.15f, 0.0f, 0.0f },
        .endRotation = { 0.1f, -0.6f, -0.9f },
        .endOffset = { 0.18f, -0.05f, 0.0f },
        .windupRatio = 0.15f,
        .strikeRatio = 0.4f,
        .duration = 0.38f,
        .sliceDirection = { 1.0f, 0.0f, 0.0f },
        .soundId = SOUND_SE_SLASH
    };

    // パターン5: 突き上げ斬り（下から上へ）
    const SlashPattern SLASH_RISING = {
        .startRotation = { 1.0f, 0.2f, 0.3f },
        .startOffset = { 0.05f, -0.15f, 0.05f },
        .endRotation = { -0.8f, -0.1f, -0.2f },
        .endOffset = { 0.0f, 0.1f, -0.05f },
        .windupRatio = 0.25f,
        .strikeRatio = 0.35f,
        .duration = 0.5f,
        .sliceDirection = { 0.0f, 1.0f, 0.0f },
        .soundId = SOUND_SE_SLASH_HEAVY
    };

    // パターン6: 鋭い縦斬り
    const SlashPattern SLASH_QUICK_VERTICAL = {
        .startRotation = { -0.7f, 0.0f, 0.0f },
        .startOffset = { 0.0f, 0.08f, 0.0f },
        .endRotation = { 1.2f, 0.0f, 0.0f },
        .endOffset = { 0.0f, -0.1f, 0.0f },
        .windupRatio = 0.15f,
        .strikeRatio = 0.35f,
        .duration = 0.35f,
        .sliceDirection = { 0.0f, -1.0f, 0.0f },
        .soundId = SOUND_SE_SLASH
    };

    // パターン配列
    const SlashPattern* SLASH_PATTERNS[] = {
        &SLASH_KESA,
        &SLASH_REVERSE_KESA,
        &SLASH_RIGHT_SWEEP,
        &SLASH_LEFT_SWEEP,
        &SLASH_RISING,
        &SLASH_QUICK_VERTICAL
    };
    constexpr int NUM_SLASH_PATTERNS = sizeof(SLASH_PATTERNS) / sizeof(SLASH_PATTERNS[0]);

    XMFLOAT3 BLADE_TIP_OFFSET = { 0.0f, 0.02f, -0.6f };
    constexpr XMFLOAT4 TRAIL_COLOR = { 0.2f, 0.5f, 1.0f, 0.4f };
    constexpr float TRAIL_SIZE = 0.25f;
    constexpr double TRAIL_LIFETIME = 0.15;
    constexpr float TRAIL_SPAWN_THRESHOLD = 2.0f;

#ifdef _DEBUG
    constexpr float DEBUG_ADJUST_SPEED = 0.02f;
#endif
}

//======================================
// イージング関数
//======================================
namespace Easing
{
    inline float OutQuad(float t)
    {
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

    inline float InOutExpo(float t)
    {
        if (t < 0.5f)
            return (powf(2.0f, 20.0f * t - 10.0f)) / 2.0f;
        return (2.0f - powf(2.0f, -20.0f * t + 10.0f)) / 2.0f;
    }

    inline float OutCubic(float t)
    {
        return 1.0f - powf(1.0f - t, 3.0f);
    }

    inline float OutBack(float t)
    {
        constexpr float c1 = 1.70158f;
        constexpr float c3 = c1 + 1.0f;
        return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
    }
}

//======================================
// 内部変数
//======================================
static MODEL* g_pModel = nullptr;
static BladeState g_State = BladeState::Idle;

static XMFLOAT3 g_ScreenOffset;
static XMFLOAT3 g_LocalPosition;
static XMFLOAT3 g_LocalRotation;
static XMFLOAT3 g_ModelRotationOffset;
static float g_Scale;

static float g_StateTimer = 0.0f;
static float g_IdleSwayTime = 0.0f;

static bool g_IsDragging = false;
static float g_SliceMouseAccumX = 0.0f;
static float g_SliceMouseAccumY = 0.0f;

// 斬撃パターン管理
static int g_CurrentPatternIndex = 0;
static int g_LastPatternIndex = -1;
static std::mt19937 g_Rng;

static XMMATRIX g_BladeWorldMatrix = XMMatrixIdentity();

#ifdef _DEBUG
static int g_DebugMode = 0;
static XMFLOAT3 g_DebugRayOrigin;
static XMFLOAT3 g_DebugRayEnd;
static XMFLOAT3 g_DebugRayEnd2;
static bool g_DebugRayValid = false;
#endif

//======================================
// 内部関数プロトタイプ
//======================================
static void ChangeState(BladeState newState);
static void UpdateState_Idle(float dt);
static void UpdateState_FreeSlice(float dt);
static void UpdateState_StylishSlash(float dt);
static void UpdateState_HorizontalSlash(float dt);
static XMFLOAT3 GetBladeTipWorldPosition();
static void SpawnTrailAtBladeTip();
static XMFLOAT3 GetCameraUp();
static void PerformSlice(const XMFLOAT3& sliceNormal, const SliceParams& params);
static int SelectNextPattern();
static XMFLOAT3 LerpFloat3(const XMFLOAT3& a, const XMFLOAT3& b, float t);
static XMFLOAT3 GetWorldSliceDirection(const XMFLOAT3& localDir);

#ifdef _DEBUG
static void DebugUpdate();
#endif

//======================================
// ユーティリティ関数
//======================================
static XMFLOAT3 LerpFloat3(const XMFLOAT3& a, const XMFLOAT3& b, float t)
{
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

static XMFLOAT3 GetWorldSliceDirection(const XMFLOAT3& localDir)
{
    XMFLOAT3 camRight = PLCamera_GetRight();
    XMFLOAT3 camUp = GetCameraUp();
    XMFLOAT3 camFront = PLCamera_GetFront();

    return {
        camRight.x * localDir.x + camUp.x * localDir.y + camFront.x * localDir.z,
        camRight.y * localDir.x + camUp.y * localDir.y + camFront.y * localDir.z,
        camRight.z * localDir.x + camUp.z * localDir.y + camFront.z * localDir.z
    };
}

static int SelectNextPattern()
{
    std::uniform_int_distribution<int> dist(0, NUM_SLASH_PATTERNS - 1);
    int next = dist(g_Rng);

    // 連続で同じパターンを避ける（最大3回試行）
    for (int i = 0; i < 3 && next == g_LastPatternIndex; ++i)
    {
        next = dist(g_Rng);
    }

    g_LastPatternIndex = next;
    return next;
}

//======================================
// 初期化
//======================================
void Blade_Initialize()
{
    g_pModel = ModelLoad(MODEL_PATH, MODEL_SCALE);

    g_State = BladeState::Idle;
    g_StateTimer = 0.0f;
    g_IdleSwayTime = 0.0f;
    g_IsDragging = false;
    g_SliceMouseAccumX = 0.0f;
    g_SliceMouseAccumY = 0.0f;

    g_ScreenOffset = DEFAULT_OFFSET;
    g_LocalPosition = { 0.0f, 0.0f, 0.0f };
    g_LocalRotation = { 0.0f, 0.0f, 0.0f };
    g_ModelRotationOffset = MODEL_ROTATION_OFFSET;
    g_Scale = MODEL_SCALE;

    // 乱数初期化
    std::random_device rd;
    g_Rng.seed(rd());
    g_LastPatternIndex = -1;

#ifdef _DEBUG
    g_DebugRayValid = false;
#endif
}

//======================================
// 終了
//======================================
void Blade_Finalize()
{
    if (g_pModel)
    {
        ModelRelease(g_pModel);
        g_pModel = nullptr;
    }
}

//======================================
// 状態遷移
//======================================
static void ChangeState(BladeState newState)
{
    if (g_State == newState) return;

    g_State = newState;
    g_StateTimer = 0.0f;

    switch (newState)
    {
        case BladeState::Idle:
            g_LocalRotation = { 0.0f, 0.0f, 0.0f };
            g_LocalPosition = { 0.0f, 0.0f, 0.0f };
            g_ModelRotationOffset = MODEL_ROTATION_OFFSET;
            break;

        case BladeState::FreeSlice:
            g_SliceMouseAccumX = 0.0f;
            g_SliceMouseAccumY = 0.0f;
            SoundManager_PlaySE(SOUND_SE_SLASH);
            break;

        case BladeState::FixedAttack:
            g_CurrentPatternIndex = SelectNextPattern();
            SoundManager_PlaySE(SLASH_PATTERNS[g_CurrentPatternIndex]->soundId);
            break;

        case BladeState::HorizontalSlash:
            g_LocalRotation = { 0.0f, 0.0f, 0.0f };
            SoundManager_PlaySE(SOUND_SE_SLASH);
            break;
    }
}

//======================================
// カメラのUp方向を取得
//======================================
static XMFLOAT3 GetCameraUp()
{
    XMFLOAT3 camFront = PLCamera_GetFront();
    XMFLOAT3 camRight = PLCamera_GetRight();
    XMVECTOR vUp = XMVector3Cross(XMLoadFloat3(&camFront), XMLoadFloat3(&camRight));

    XMFLOAT3 camUp;
    XMStoreFloat3(&camUp, vUp);
    return camUp;
}

//======================================
// 状態別更新: 待機
//======================================
static void UpdateState_Idle(float dt)
{
    g_IdleSwayTime += dt * IDLE_SWAY_SPEED;
    g_LocalPosition.x = sinf(g_IdleSwayTime) * IDLE_SWAY_AMOUNT;
    g_LocalPosition.y = cosf(g_IdleSwayTime * 0.7f) * IDLE_SWAY_AMOUNT * 0.5f;

    const Mouse_State& ms = InputManager_GetMouseState();

    if (ms.rightButton)
    {
        g_IsDragging = true;
        ChangeState(BladeState::FreeSlice);
    }
    else if (ms.leftButton)
    {
        ChangeState(BladeState::FixedAttack);
    }
}

//======================================
// 状態別更新: 自由斬撃
//======================================
static void UpdateState_FreeSlice(float dt)
{
    (void)dt;

    const Mouse_State& ms = InputManager_GetMouseState();

    float deltaX = static_cast<float>(ms.x);
    float deltaY = static_cast<float>(ms.y);

    g_SliceMouseAccumX += deltaX;
    g_SliceMouseAccumY += deltaY;

    g_LocalRotation.z = g_SliceMouseAccumX * SLICE_ROTATION_SENSITIVITY;
    g_LocalRotation.x = -g_SliceMouseAccumY * SLICE_ROTATION_SENSITIVITY;

    float swingSpeed = sqrtf(deltaX * deltaX + deltaY * deltaY);

    if (swingSpeed > TRAIL_SPAWN_THRESHOLD)
    {
        SpawnTrailAtBladeTip();
    }

    if (swingSpeed > SLICE_SWING_THRESHOLD)
    {
        XMFLOAT3 camRight = PLCamera_GetRight();
        XMFLOAT3 camUp = GetCameraUp();

        float normalizedX = deltaX / (swingSpeed + 0.001f);
        float normalizedY = deltaY / (swingSpeed + 0.001f);

        XMVECTOR vSliceNormal = XMLoadFloat3(&camRight) * normalizedY - XMLoadFloat3(&camUp) * normalizedX;
        vSliceNormal = XMVector3Normalize(vSliceNormal);

        XMFLOAT3 sliceNormal;
        XMStoreFloat3(&sliceNormal, vSliceNormal);

        PerformSlice(sliceNormal, DEFAULT_SLICE);
    }

    if (ms.leftButton)
    {
        g_IsDragging = false;
        ChangeState(BladeState::FixedAttack);
        return;
    }

    if (!ms.rightButton)
    {
        g_IsDragging = false;
        ChangeState(BladeState::Idle);
    }
}

//======================================
// 状態別更新: スタイリッシュ斬撃
//======================================
static void UpdateState_StylishSlash(float dt)
{
    const SlashPattern& pattern = *SLASH_PATTERNS[g_CurrentPatternIndex];

    g_StateTimer += dt;
    float t = g_StateTimer / pattern.duration;

    float windupEnd = pattern.windupRatio;
    float strikeEnd = pattern.windupRatio + pattern.strikeRatio;

    XMFLOAT3 targetRot;
    XMFLOAT3 targetOffset;

    if (t < windupEnd)
    {
        // 振りかぶりフェーズ
        float phaseT = t / windupEnd;
        float eased = Easing::OutQuad(phaseT);

        targetRot = LerpFloat3({ 0.0f, 0.0f, 0.0f }, pattern.startRotation, eased);
        targetOffset = LerpFloat3({ 0.0f, 0.0f, 0.0f }, pattern.startOffset, eased);
    }
    else if (t < strikeEnd)
    {
        // 斬撃フェーズ
        float phaseT = (t - windupEnd) / pattern.strikeRatio;
        float eased = Easing::InOutExpo(phaseT);

        targetRot = LerpFloat3(pattern.startRotation, pattern.endRotation, eased);
        targetOffset = LerpFloat3(pattern.startOffset, pattern.endOffset, eased);

        // トレイル生成
        SpawnTrailAtBladeTip();

        // 切断チェック（毎フレーム実行）
        XMFLOAT3 worldSliceDir = GetWorldSliceDirection(pattern.sliceDirection);
        XMVECTOR vDir = XMVector3Normalize(XMLoadFloat3(&worldSliceDir));
        XMStoreFloat3(&worldSliceDir, vDir);

        PerformSlice(worldSliceDir, DEFAULT_SLICE);
    }
    else
    {
        // リカバリーフェーズ
        float phaseT = (t - strikeEnd) / (1.0f - strikeEnd);
        float eased = Easing::OutCubic(phaseT);

        targetRot = LerpFloat3(pattern.endRotation, { 0.0f, 0.0f, 0.0f }, eased);
        targetOffset = LerpFloat3(pattern.endOffset, { 0.0f, 0.0f, 0.0f }, eased);
    }

    g_LocalRotation = targetRot;
    g_LocalPosition = targetOffset;

    if (g_StateTimer >= pattern.duration)
    {
        ChangeState(BladeState::Idle);
    }
}

//======================================
// 状態別更新: 横なぎ攻撃（AirDash専用）
//======================================
static void UpdateState_HorizontalSlash(float dt)
{
    g_StateTimer += dt;
    float t = g_StateTimer / HORIZONTAL_SLASH_DURATION;

    // イーズアウト（最初速く、後半減速）
    float easeT = 1.0f - (1.0f - t) * (1.0f - t);

    g_ModelRotationOffset.x = HORIZONTAL_ROT_X;
    g_ModelRotationOffset.y = HORIZONTAL_ROT_Y_START + easeT * (HORIZONTAL_ROT_Y_END - HORIZONTAL_ROT_Y_START);
    g_ModelRotationOffset.z = HORIZONTAL_ROT_Z;

    g_LocalRotation = { 0.0f, 0.0f, 0.0f };

    // トレイル生成
    SpawnTrailAtBladeTip();

    // 切断チェック（斬撃フェーズ中は毎フレーム実行）
    if (t > 0.15f && t < 0.85f)
    {
        PerformSlice(GetCameraUp(), HORIZONTAL_SLICE);
    }

    if (g_StateTimer >= HORIZONTAL_SLASH_DURATION)
    {
        g_ModelRotationOffset = MODEL_ROTATION_OFFSET;
        ChangeState(BladeState::Idle);
    }
}

//======================================
// デバッグ更新
//======================================
#ifdef _DEBUG
static void DebugUpdate()
{
    if (KeyLogger_IsTrigger(KK_TAB))
    {
        g_DebugMode = (g_DebugMode + 1) % 2;
    }

    if (g_DebugMode == 0)
    {
        if (KeyLogger_IsPressed(KK_T)) g_ModelRotationOffset.x += DEBUG_ADJUST_SPEED;
        if (KeyLogger_IsPressed(KK_G)) g_ModelRotationOffset.x -= DEBUG_ADJUST_SPEED;
        if (KeyLogger_IsPressed(KK_Y)) g_ModelRotationOffset.y += DEBUG_ADJUST_SPEED;
        if (KeyLogger_IsPressed(KK_H)) g_ModelRotationOffset.y -= DEBUG_ADJUST_SPEED;
        if (KeyLogger_IsPressed(KK_U)) g_ModelRotationOffset.z += DEBUG_ADJUST_SPEED;
        if (KeyLogger_IsPressed(KK_J)) g_ModelRotationOffset.z -= DEBUG_ADJUST_SPEED;
    }
    else
    {
        if (KeyLogger_IsPressed(KK_T)) BLADE_TIP_OFFSET.x += DEBUG_ADJUST_SPEED;
        if (KeyLogger_IsPressed(KK_G)) BLADE_TIP_OFFSET.x -= DEBUG_ADJUST_SPEED;
        if (KeyLogger_IsPressed(KK_Y)) BLADE_TIP_OFFSET.y += DEBUG_ADJUST_SPEED;
        if (KeyLogger_IsPressed(KK_H)) BLADE_TIP_OFFSET.y -= DEBUG_ADJUST_SPEED;
        if (KeyLogger_IsPressed(KK_U)) BLADE_TIP_OFFSET.z += DEBUG_ADJUST_SPEED;
        if (KeyLogger_IsPressed(KK_J)) BLADE_TIP_OFFSET.z -= DEBUG_ADJUST_SPEED;

        if (KeyLogger_IsPressed(KK_SPACE))
        {
            SpawnTrailAtBladeTip();
        }
    }
}
#endif

//======================================
// 更新
//======================================
void Blade_Update(double elapsed_time)
{
#ifdef _DEBUG
    DebugUpdate();
#endif

    float dt = static_cast<float>(elapsed_time);

    switch (g_State)
    {
        case BladeState::Idle:
            UpdateState_Idle(dt);
            break;
        case BladeState::FreeSlice:
            UpdateState_FreeSlice(dt);
            break;
        case BladeState::FixedAttack:
            UpdateState_StylishSlash(dt);
            break;
        case BladeState::HorizontalSlash:
            UpdateState_HorizontalSlash(dt);
            break;
    }
}

//======================================
// 描画
//======================================
void Blade_Draw()
{
    if (!g_pModel) return;

    XMFLOAT4X4 viewMat = PLCamera_GetViewMatrix();
    XMMATRIX mtxCameraWorld = XMMatrixInverse(nullptr, XMLoadFloat4x4(&viewMat));

    XMMATRIX mtxScale = XMMatrixScaling(g_Scale, g_Scale, g_Scale);

    XMMATRIX mtxModelOffset = XMMatrixRotationRollPitchYaw(
        g_ModelRotationOffset.x, g_ModelRotationOffset.y, g_ModelRotationOffset.z
    );

    XMMATRIX mtxLocalRot = XMMatrixRotationRollPitchYaw(
        g_LocalRotation.x, g_LocalRotation.y, g_LocalRotation.z
    );

    XMFLOAT3 localPos = {
        g_ScreenOffset.x + g_LocalPosition.x,
        g_ScreenOffset.y + g_LocalPosition.y,
        g_ScreenOffset.z + g_LocalPosition.z
    };
    XMMATRIX mtxLocalTrans = XMMatrixTranslation(localPos.x, localPos.y, localPos.z);

    XMMATRIX mtxWorld = mtxScale * mtxModelOffset * mtxLocalRot * mtxLocalTrans * mtxCameraWorld;

    g_BladeWorldMatrix = mtxWorld;

    ModelDraw(g_pModel, mtxWorld);
}

//======================================
// シャドウマップ描画
//======================================
void Blade_DrawShadow()
{
    if (!g_pModel) return;
    ModelDrawShadow(g_pModel, g_BladeWorldMatrix);
}

//======================================
// ブレード先端のワールド位置を取得
//======================================
static XMFLOAT3 GetBladeTipWorldPosition()
{
    XMVECTOR vTipLocal = XMLoadFloat3(&BLADE_TIP_OFFSET);
    XMVECTOR vTipWorld = XMVector3TransformCoord(vTipLocal, g_BladeWorldMatrix);

    XMFLOAT3 tipWorld;
    XMStoreFloat3(&tipWorld, vTipWorld);
    return tipWorld;
}

//======================================
// トレイル生成
//======================================
static void SpawnTrailAtBladeTip()
{
    Trail_Create(GetBladeTipWorldPosition(), TRAIL_COLOR, TRAIL_SIZE, TRAIL_LIFETIME);
}

//======================================
// 切断を実行
//======================================
static void PerformSlice(const XMFLOAT3& sliceNormal, const SliceParams& params)
{
    XMFLOAT3 rayOrigin = GetBladeTipWorldPosition();

    XMVECTOR vLocalDir = XMLoadFloat3(&params.rayDirection);
    XMVECTOR vWorldDir = XMVector3Normalize(XMVector3TransformNormal(vLocalDir, g_BladeWorldMatrix));

    XMFLOAT3 rayDir;
    XMStoreFloat3(&rayDir, vWorldDir);

    XMFLOAT3 dir1 = {
        rayDir.x * params.rayLength,
        rayDir.y * params.rayLength,
        rayDir.z * params.rayLength
    };

    XMFLOAT3 dir2 = {
        dir1.x + sliceNormal.x * params.planeSpread,
        dir1.y + sliceNormal.y * params.planeSpread,
        dir1.z + sliceNormal.z * params.planeSpread
    };

#ifdef _DEBUG
    g_DebugRayOrigin = rayOrigin;
    g_DebugRayEnd = { rayOrigin.x + dir1.x, rayOrigin.y + dir1.y, rayOrigin.z + dir1.z };
    g_DebugRayEnd2 = { rayOrigin.x + dir2.x, rayOrigin.y + dir2.y, rayOrigin.z + dir2.z };
    g_DebugRayValid = true;
#endif

    Ray startRay(rayOrigin, dir1);
    Ray endRay(rayOrigin, dir2);

    PropManager_TrySlice(startRay, endRay);
    Enemy_TrySlice(endRay, sliceNormal);
}

//======================================
// デバッグ描画
//======================================
void Blade_DebugDraw()
{
#ifdef _DEBUG
    XMFLOAT3 rayOrigin = GetBladeTipWorldPosition();

    XMVECTOR vLocalDir = XMLoadFloat3(&DEFAULT_SLICE.rayDirection);
    XMVECTOR vWorldDir = XMVector3Normalize(XMVector3TransformNormal(vLocalDir, g_BladeWorldMatrix));
    XMFLOAT3 rayDir;
    XMStoreFloat3(&rayDir, vWorldDir);

    XMFLOAT3 rayEnd = {
        rayOrigin.x + rayDir.x * DEFAULT_SLICE.rayLength,
        rayOrigin.y + rayDir.y * DEFAULT_SLICE.rayLength,
        rayOrigin.z + rayDir.z * DEFAULT_SLICE.rayLength
    };

    DebugRenderer::DrawLine(rayOrigin, rayEnd, { 0.0f, 1.0f, 0.0f, 1.0f });
    DebugRenderer::DrawSphere({ rayOrigin, 0.05f }, { 1.0f, 1.0f, 0.0f, 1.0f });
    DebugRenderer::DrawSphere({ rayEnd, 0.05f }, { 1.0f, 0.0f, 0.0f, 1.0f });

    if (g_DebugRayValid && Blade_IsAttacking())
    {
        DebugRenderer::DrawLine(g_DebugRayOrigin, g_DebugRayEnd, { 1.0f, 0.0f, 1.0f, 1.0f });
        DebugRenderer::DrawLine(g_DebugRayOrigin, g_DebugRayEnd2, { 1.0f, 0.5f, 1.0f, 1.0f });
    }
#endif
}

//======================================
// ゲッター
//======================================
BladeState Blade_GetState()
{
    return g_State;
}

bool Blade_IsAttacking()
{
    return g_State == BladeState::FreeSlice ||
        g_State == BladeState::FixedAttack ||
        g_State == BladeState::HorizontalSlash;
}

int Blade_GetDebugMode()
{
#ifdef _DEBUG
    return g_DebugMode;
#else
    return 0;
#endif
}

int Blade_GetCurrentPatternIndex()
{
    return g_CurrentPatternIndex;
}

//======================================
// 設定
//======================================
void Blade_SetScreenOffset(const XMFLOAT3& offset)
{
    g_ScreenOffset = offset;
}

void Blade_SetScale(float scale)
{
    g_Scale = scale;
}

//======================================
// 外部トリガー
//======================================
void Blade_TriggerVerticalSlash()
{
    if (g_State == BladeState::Idle || g_State == BladeState::FreeSlice)
    {
        ChangeState(BladeState::FixedAttack);
    }
}

void Blade_TriggerHorizontalSlash()
{
    ChangeState(BladeState::HorizontalSlash);
}