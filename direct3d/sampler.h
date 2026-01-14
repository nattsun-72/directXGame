/**
 * @file sampler.h
 * @brief サンプラーの設定ユーティリティー
 * @author Natsume Shidara
 * @date 2025/09/18
 */
#ifndef SAMPLER_H
#define SAMPLER_H

#include <d3d11.h>

void Sampler_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Sampler_Finalize(void);

void Sampler_SetFilterPoint();
void Sampler_SetFilterLinear();
void Sampler_SetFilterAnisotropic();

#endif // SAMPLER_H
