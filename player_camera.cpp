/**
 * @file player_camera.cpp
 * @brief プレイヤーカメラ制御（一人称視点）
 *
 * @author Natsume Shidara
 * @date 2025/10/31
 * @update 2026/01/07 - 一人称視点に変更
 */
#include "player_camera.h"
#include <DirectXMath.h>
#include <cmath>
#include "player.h"
#include "direct3d.h"
#include "input_manager.h"
#include "pad_logger.h"
#include "blade.h"
#include "game_settings.h"

using namespace DirectX;

//======================================
// 定数
//======================================
static constexpr float DEFAULT_FOV = 1.0f;          // デフォルトFOV（約57度）
static constexpr float MIN_FOV = 0.5f;
static constexpr float MAX_FOV = 2.0f;

static const float DEFAULT_SENSITIVITY = GameSettings_GetSensitivity() * 0.005f; // マウス感度
static constexpr float PAD_SENSITIVITY = 2.5f;       // パッド右スティック感度
static constexpr float PAD_DEADZONE = 0.15f;         // パッドデッドゾーン
static constexpr float MIN_PITCH = -XM_PIDIV2 + 0.1f; // 下限（約-80度）
static constexpr float MAX_PITCH = XM_PIDIV2 - 0.1f;  // 上限（約+80度）

//======================================
// 内部変数
//======================================
static XMFLOAT3 g_CameraPosition = { 0.0f, 0.0f, 0.0f };
static XMFLOAT3 g_CameraFront = { 0.0f, 0.0f, 1.0f };
static XMFLOAT3 g_CameraRight = { 1.0f, 0.0f, 0.0f };
static XMFLOAT3 g_CameraUp = { 0.0f, 1.0f, 0.0f };

static XMFLOAT3 g_EyeOffset = { 0.0f, 1.6f, 0.0f }; // プレイヤー頭の高さ

static float g_Yaw = 0.0f;      // 左右回転（ラジアン）
static float g_Pitch = 0.0f;    // 上下回転（ラジアン）

static float g_MouseSensitivity = DEFAULT_SENSITIVITY;

static XMFLOAT4X4 g_ViewMatrix{};
static XMFLOAT4X4 g_PerspectiveMatrix{};

// FOV制御
static float g_CurrentFOV = DEFAULT_FOV;
static float g_TargetFOV = DEFAULT_FOV;
static float g_FOVSpeed = 10.0f;

// 初回更新フラグ
static bool g_FirstUpdate = true;

//======================================
// 初期化
//======================================
void PLCamera_Initialize(XMFLOAT3 eyeOffset)
{
    g_EyeOffset = eyeOffset;

    // 角度初期化
    g_Yaw = 0.0f;
    g_Pitch = 0.0f;

    // FOV初期化
    g_CurrentFOV = DEFAULT_FOV;
    g_TargetFOV = DEFAULT_FOV;
    g_FOVSpeed = 10.0f;

    g_MouseSensitivity = DEFAULT_SENSITIVITY;
    g_FirstUpdate = true;

    // 初期ビュー行列
    XMMATRIX identity = XMMatrixIdentity();
    XMStoreFloat4x4(&g_ViewMatrix, identity);

    // マウスを相対座標モードに設定（FPS向け）
    Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
    Mouse_SetVisible(false);
}

//======================================
// 終了
//======================================
void PLCamera_Finalize(void)
{
    // マウスを通常モードに戻す
    Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
    Mouse_SetVisible(true);
}

//======================================
// 更新
//======================================
void PLCamera_Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);

    //--------------------------------------
    // マウス・パッド入力で視点回転
    //--------------------------------------
    // FreeSlice中はカメラ回転をスキップ（ブレードがマウス入力を使用）
    bool skipMouseInput = (Blade_GetState() == BladeState::FreeSlice);

    if (!g_FirstUpdate && !skipMouseInput)
    {
        float deltaX = 0.0f;
        float deltaY = 0.0f;

        // マウス入力
        const Mouse_State& mouseState = InputManager_GetMouseState();
        deltaX += static_cast<float>(mouseState.x) * g_MouseSensitivity;
        deltaY += static_cast<float>(mouseState.y) * g_MouseSensitivity;

        // パッド右スティック入力
        XMFLOAT2 rightStick = PadLogger_GetRightThumbStick(0);
        if (isfinite(rightStick.x) && isfinite(rightStick.y))
        {
            // デッドゾーン処理
            if (fabsf(rightStick.x) > PAD_DEADZONE)
            {
                deltaX += rightStick.x * PAD_SENSITIVITY * dt;
            }
            if (fabsf(rightStick.y) > PAD_DEADZONE)
            {
                deltaY -= rightStick.y * PAD_SENSITIVITY * dt;  // Y軸反転
            }
        }

        // Yaw（左右）とPitch（上下）を更新
        g_Yaw += deltaX;
        g_Pitch -= deltaY;

        // Yawを0〜2πに正規化
        while (g_Yaw > XM_2PI) g_Yaw -= XM_2PI;
        while (g_Yaw < 0.0f) g_Yaw += XM_2PI;

        // Pitchを制限（真上・真下を向けないように）
        g_Pitch = fmaxf(MIN_PITCH, fminf(MAX_PITCH, g_Pitch));
    }
    g_FirstUpdate = false;

    //--------------------------------------
    // カメラの方向ベクトルを計算
    //--------------------------------------
    XMVECTOR vFront;
    vFront = XMVectorSet(
        cosf(g_Pitch) * sinf(g_Yaw),
        sinf(g_Pitch),
        cosf(g_Pitch) * cosf(g_Yaw),
        0.0f
    );
    vFront = XMVector3Normalize(vFront);

    XMVECTOR vWorldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR vRight = XMVector3Normalize(XMVector3Cross(vWorldUp, vFront));
    XMVECTOR vUp = XMVector3Cross(vFront, vRight);

    XMStoreFloat3(&g_CameraFront, vFront);
    XMStoreFloat3(&g_CameraRight, vRight);
    XMStoreFloat3(&g_CameraUp, vUp);

    //--------------------------------------
    // カメラ位置（プレイヤー位置 + 目の高さ）
    //--------------------------------------
    const XMFLOAT3& playerPos = Player_GetPosition();

    g_CameraPosition.x = playerPos.x + g_EyeOffset.x;
    g_CameraPosition.y = playerPos.y + g_EyeOffset.y;
    g_CameraPosition.z = playerPos.z + g_EyeOffset.z;

    XMVECTOR vCameraPos = XMLoadFloat3(&g_CameraPosition);

    //--------------------------------------
    // ビュー行列
    //--------------------------------------
    XMVECTOR vTarget = XMVectorAdd(vCameraPos, vFront);
    XMMATRIX mtxView = XMMatrixLookAtLH(vCameraPos, vTarget, vWorldUp);
    XMStoreFloat4x4(&g_ViewMatrix, mtxView);

    //--------------------------------------
    // FOV補間
    //--------------------------------------
    if (g_FOVSpeed > 0.0f && fabsf(g_CurrentFOV - g_TargetFOV) > 0.001f)
    {
        float diff = g_TargetFOV - g_CurrentFOV;
        float change = g_FOVSpeed * dt;

        if (fabsf(diff) <= change)
        {
            g_CurrentFOV = g_TargetFOV;
        }
        else
        {
            g_CurrentFOV += (diff > 0.0f ? change : -change);
        }
    }
    g_CurrentFOV = fmaxf(MIN_FOV, fminf(MAX_FOV, g_CurrentFOV));

    //--------------------------------------
    // 投影行列
    //--------------------------------------
    float aspectRatio = static_cast<float>(Direct3D_GetBackBufferWidth()) /
        static_cast<float>(Direct3D_GetBackBufferHeight());
    float nearZ = 0.01f;
    float farZ = 10000.0f;

    XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(g_CurrentFOV, aspectRatio, nearZ, farZ);
    XMStoreFloat4x4(&g_PerspectiveMatrix, mtxPerspective);
}

//======================================
// ゲッター
//======================================
const XMFLOAT3& PLCamera_GetPosition() { return g_CameraPosition; }
const XMFLOAT3& PLCamera_GetFront() { return g_CameraFront; }
const XMFLOAT3& PLCamera_GetRight() { return g_CameraRight; }

const XMFLOAT4X4& PLCamera_GetViewMatrix() { return g_ViewMatrix; }
const XMFLOAT4X4& PLCamera_GetPerspectiveMatrix() { return g_PerspectiveMatrix; }

float PLCamera_GetYaw() { return g_Yaw; }
float PLCamera_GetPitch() { return g_Pitch; }

//======================================
// FOV制御
//======================================
void PLCamera_SetTargetFOV(float fov, float speed)
{
    g_TargetFOV = fmaxf(MIN_FOV, fminf(MAX_FOV, fov));
    g_FOVSpeed = speed;

    if (speed <= 0.0f)
    {
        g_CurrentFOV = g_TargetFOV;
    }
}

void PLCamera_SetFOV(float fov)
{
    g_CurrentFOV = fmaxf(MIN_FOV, fminf(MAX_FOV, fov));
    g_TargetFOV = g_CurrentFOV;
}

float PLCamera_GetFOV()
{
    return g_CurrentFOV;
}

void PLCamera_ResetFOV(float speed)
{
    PLCamera_SetTargetFOV(DEFAULT_FOV, speed);
}

float PLCamera_GetDefaultFOV()
{
    return DEFAULT_FOV;
}

//======================================
// 感度設定
//======================================
void PLCamera_SetMouseSensitivity(float sensitivity)
{
    g_MouseSensitivity = fmaxf(0.0001f, sensitivity);
}

float PLCamera_GetMouseSensitivity()
{
    return g_MouseSensitivity;
}