/**
 * @file camera.h
 * @brief ÉJÉÅÉâêßå‰
 * 
 * @author Natsume Shidara
 * @date 2025/09/11
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <DirectXMath.h>

void Camera_Initialize(
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT3& front,
    const DirectX::XMFLOAT3& up
    );

void Camera_Initialize();
void Camera_Finalize(void);
void Camera_Update(double elapsed_time);

const DirectX::XMFLOAT4X4& Camera_GetViewMatrix();
const DirectX::XMFLOAT4X4& Camera_GetPerspectiveMatrix();

const DirectX::XMFLOAT3& Camera_GetFront();
const DirectX::XMFLOAT3& Camera_GetRight();
const DirectX::XMFLOAT3& Camera_GetUp();
const float& Camera_GetFov();

const DirectX::XMFLOAT3& Camera_GetPosition();

void Camera_DebugDraw();

void Camera_SetMatrix(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection);

#endif // CAMERA_H
