/**
 * @file billboard.h
 * @brief スプライト表示
 * @author Natsume Shidara
 * @date 2025/06/12
 */

#ifndef BILLBOARD_H
#define BILLBOARD_H

#include <DirectXMath.h>
#include <d3d11.h>
#include "collision.h"

void Billboard_Initialize();
void Billboard_Finalize(void);
void Billboard_Update(double elapsed_time);
void Billboard_Draw(int texId, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT2& pivot = { 0.0f, 0.0f }, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

void Billboard_Draw(int texId, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT4& tex_cut, const DirectX::XMFLOAT2& pivot = { 0.0f, 0.0f }, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

void Billboard_SetViewMatrix(const DirectX::XMFLOAT4X4& view);

#endif // BILLBOARD_H
