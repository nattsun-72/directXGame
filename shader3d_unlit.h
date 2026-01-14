/**
 * @file shader3d_unlit.h
 * @brief 3D用アンリットシェーダー
 * @author Natsume Shidara
 * @date 2025/11/21
 */

#ifndef SHADER3D_UNLIT_H
#define SHADER3D_UNLIT_H

#include <DirectXMath.h>
#include <d3d11.h>

void Shader3D_Unlit_Initialize();
void Shader3D_Unlit_Finalize();
			
void Shader3D_Unlit_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
			
void Shader3D_Unlit_SetColor(const DirectX::XMFLOAT4& color);
			
void Shader3D_Unlit_Begin();

#endif // SHADER3D_UNLIT_H