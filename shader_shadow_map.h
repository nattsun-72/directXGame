/****************************************
 * @file shader_shadow_map.h
 * @brief シャドウマップ生成用シェーダー管理
 * @author Natsume Shidara
 * @date 2025/12/10
 ****************************************/

#ifndef SHADER_SHADOW_MAP_H
#define SHADER_SHADOW_MAP_H

#include <d3d11.h>
#include <DirectXMath.h>

 // 初期化・終了
bool ShaderShadowMap_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void ShaderShadowMap_Finalize();

// 描画開始
void ShaderShadowMap_Begin();

// 定数バッファ更新
// ※ HLSLのcbuffer定義に合わせて、LightViewProjとWorldをセットする
void ShaderShadowMap_SetLightViewProjection(const DirectX::XMFLOAT4X4& matrix);
void ShaderShadowMap_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

#endif // SHADER_SHADOW_MAP_H