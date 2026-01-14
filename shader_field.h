/**
 * @file shader_field.h
 * @brief フィールド用シェーダー
 * @author Natsume Shidara
 * @date 2025/09/26
 */

#ifndef SHADERF_IELD_H
#define SHADER_FIELD_H

#include <DirectXMath.h>
#include <d3d11.h>

bool ShaderField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void ShaderField_Finalize();

void ShaderField_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

void ShaderField_SetColor(const DirectX::XMFLOAT4& color);

void ShaderField_Begin();
#endif // SHADER_FIELD_H
