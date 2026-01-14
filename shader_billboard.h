/**
 * @file shader_billboard.h
 * @brief シェーダー
 * @author Natsume Shidara
 * @date 2025/06/10
 */

#ifndef SHADER_BILLBOARD_H
#define SHADER_BILLBOARD_H

#include <DirectXMath.h>
#include <d3d11.h>

struct UVParameter
{
    DirectX::XMFLOAT2 scale;
    DirectX::XMFLOAT2 translation;
};

bool Shader_Billboard_Initialize();
void Shader_Billboard_Finalize();

void Shader_Billboard_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

void Shader_Billboard_SetColor(const DirectX::XMFLOAT4& color);

void Shader_Billboard_SetUVParameter(const UVParameter& parameter);

void Shader_Billboard_Begin();

#endif // SHADER_BILLBOARD_H
