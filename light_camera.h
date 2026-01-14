#ifndef LIGHT_CAMERA_H
#define LIGHT_CAMERA_H

#include <DirectXMath.h>

// 初期化（引数をcppに合わせました）
void LightCamera_Initialize(const DirectX::XMFLOAT3& world_directional, const DirectX::XMFLOAT3& world_Position);

// 終了処理
void LightCamera_Finalize();

// 設定関連
void LightCamera_SetPosition(const DirectX::XMFLOAT3& position); // 変数名のタイポ修正(posiiton -> position)
void LightCamera_SetFront(const DirectX::XMFLOAT3& front);


void LightCamera_SetRange(float w, float h, float n, float f);


void LightCamera_Update();

// 取得関連
const DirectX::XMFLOAT4X4& LightCamera_GetViewMatrix();
const DirectX::XMFLOAT4X4& LightCamera_GetProjectionMatrix();

#endif // LIGHT_CAMERA_H