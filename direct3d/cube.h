/**
 * @file cube.h
 * @brief 3Dキューブの表示
 * @author Natsume Shidara
 * @date 2025/09/09
 */
#ifndef CUBE_H
#define CUBE_H

#include <d3d11.h>
#include <DirectXMath.h>
#include "collision.h"

void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Cube_Finalize(void);
void Cube_Update(double elapsed_time);

void Cube_Draw(int texId, const DirectX::XMMATRIX& mtxWorld = DirectX::XMMatrixIdentity(), const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

AABB Cube_GetAABB(const DirectX::XMFLOAT3& position);

// 影生成用
void Cube_DrawShadow(const DirectX::XMMATRIX& mtxWorld);

#endif // CUBE_H