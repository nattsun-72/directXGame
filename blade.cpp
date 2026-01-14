/****************************************
 * @file blade.cpp
 * @brief ブレード制御（一人称視点）
 * @author Natsume Shidara
 * @date 2026/01/07
 * @update 2026/01/10 - リファクタリング
 * @update 2026/01/13 - 切断法線計算の修正
 * @update 2026/01/13 - サウンド対応
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

    constexpr float FIXED_ATTACK_DURATION = 0.3f;
    constexpr float HORIZONTAL_SLASH_DURATION = 0.2f;

    constexpr float SLICE_ROTATION_SENSITIVITY = 0.005f;
    constexpr float SLICE_SWING_THRESHOLD = 2.0f;

    const SliceParams DEFAULT_SLICE = {
        { 0.0f, 0.0f, -1.0f },
        7.5f,
        0.4f
    };

    const SliceParams VERTICAL_SLICE = {
        { 0.0f, -0.4f, -1.0f },
        7.5f,
        0.4f
    };

    const SliceParams HORIZONTAL_SLICE = {
        { 0.0f, 0.0f, -1.0f },
        7.5f,
        0.4f
    };

    constexpr float HORIZONTAL_ROT_X = 0.0f;
    constexpr float HORIZONTAL_ROT_Z = 3.0f;
    constexpr float HORIZONTAL_ROT_Y_START = 8.0f;
    constexpr float HORIZONTAL_ROT_Y_END = 11.0f;

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
static void UpdateState_FixedAttack(float dt);
static void UpdateState_HorizontalSlash(float dt);
static XMFLOAT3 GetBladeTipWorldPosition();
static void SpawnTrailAtBladeTip();
static XMFLOAT3 GetCameraUp();
static void PerformSlice(const XMFLOAT3& sliceNormal, const SliceParams& params);

#ifdef _DEBUG
static void DebugUpdate();
#endif

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
        g_ModelRotationOffset = MODEL_ROTATION_OFFSET;
        break;

    case BladeState::FreeSlice:
        g_SliceMouseAccumX = 0.0f;
        g_SliceMouseAccumY = 0.0f;
        SoundManager_PlaySE(SOUND_SE_SLASH);
        break;

    case BladeState::FixedAttack:
        g_LocalRotation = { 0.0f, 0.0f, 0.0f };
        SoundManager_PlaySE(SOUND_SE_SLASH_HEAVY);
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
// 状態別更新: 固定攻撃（縦斬り）
//======================================
static void UpdateState_FixedAttack(float dt)
{
    g_StateTimer += dt;
    float t = g_StateTimer / FIXED_ATTACK_DURATION;

    constexpr float SWING_UP_ANGLE = -0.8f;
    constexpr float SWING_DOWN_ANGLE = 3.2f;

    if (t < 0.25f)
    {
        g_LocalRotation.x = (t / 0.25f) * SWING_UP_ANGLE;
    }
    else if (t < 0.5f)
    {
        float phase = (t - 0.25f) / 0.25f;
        g_LocalRotation.x = SWING_UP_ANGLE + phase * (SWING_DOWN_ANGLE - SWING_UP_ANGLE);
        SpawnTrailAtBladeTip();
        PerformSlice(PLCamera_GetRight(), VERTICAL_SLICE);
    }
    else
    {
        float phase = (t - 0.5f) / 0.5f;
        g_LocalRotation.x = SWING_DOWN_ANGLE * (1.0f - phase);
    }

    if (g_StateTimer >= FIXED_ATTACK_DURATION)
    {
        ChangeState(BladeState::Idle);
    }
}

//======================================
// 状態別更新: 横なぎ攻撃
//======================================
static void UpdateState_HorizontalSlash(float dt)
{
    g_StateTimer += dt;
    float t = g_StateTimer / HORIZONTAL_SLASH_DURATION;

    float easeT = 1.0f - (1.0f - t) * (1.0f - t);

    g_ModelRotationOffset.x = HORIZONTAL_ROT_X;
    g_ModelRotationOffset.y = HORIZONTAL_ROT_Y_START + easeT * (HORIZONTAL_ROT_Y_END - HORIZONTAL_ROT_Y_START);
    g_ModelRotationOffset.z = HORIZONTAL_ROT_Z;

    g_LocalRotation = { 0.0f, 0.0f, 0.0f };

    SpawnTrailAtBladeTip();

    if (t > 0.2f && t < 0.8f)
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
    case BladeState::Idle:           UpdateState_Idle(dt);           break;
    case BladeState::FreeSlice:      UpdateState_FreeSlice(dt);      break;
    case BladeState::FixedAttack:    UpdateState_FixedAttack(dt);    break;
    case BladeState::HorizontalSlash: UpdateState_HorizontalSlash(dt); break;
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