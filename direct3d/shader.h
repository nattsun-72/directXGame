/**
 * @file shader.h
 * @brief シェーダー
 * @author Natsume Shidara
 * @date 2025/06/10
 */

#ifndef SHADER_H
#define	SHADER_H

#include <d3d11.h>
#include <DirectXMath.h>

bool Shader_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Shader_Finalize();

void Shader_SetProjectionMatrix(const DirectX::XMMATRIX& matrix);
void Shader_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

void Shader_Begin();

#endif // SHADER_H

