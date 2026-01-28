/**
 * @file player_camera.h
 * @brief プレイヤーカメラ制御（一人称視点）
 *
 * @author Natsume Shidara
 * @date 2025/10/31
 * @update 2026/01/07 - 一人称視点に変更
 * @update 2026/01/28 - カメラシェイク機能追加
 */

#ifndef PLAYER_CAMERA_H
#define PLAYER_CAMERA_H

#include <DirectXMath.h>

 //======================================
 // 初期化・終了
 //======================================
void PLCamera_Initialize(DirectX::XMFLOAT3 eyeOffset);
void PLCamera_Finalize(void);

//======================================
// 更新
//======================================
void PLCamera_Update(double elapsed_time);

//======================================
// ゲッター
//======================================
const DirectX::XMFLOAT3& PLCamera_GetPosition();
const DirectX::XMFLOAT3& PLCamera_GetFront();
const DirectX::XMFLOAT3& PLCamera_GetRight();

const DirectX::XMFLOAT4X4& PLCamera_GetViewMatrix();
const DirectX::XMFLOAT4X4& PLCamera_GetPerspectiveMatrix();

float PLCamera_GetYaw();
float PLCamera_GetPitch();

//======================================
// FOV制御
//======================================
void PLCamera_SetTargetFOV(float fov, float speed);
void PLCamera_SetFOV(float fov);
float PLCamera_GetFOV();
void PLCamera_ResetFOV(float speed);
float PLCamera_GetDefaultFOV();

//======================================
// 感度設定
//======================================
void PLCamera_SetMouseSensitivity(float sensitivity);
float PLCamera_GetMouseSensitivity();

//======================================
// カメラシェイク
//======================================
void PLCamera_Shake(float shakePower);
void PLCamera_StopShake();
bool PLCamera_IsShaking();

#endif // PLAYER_CAMERA_H