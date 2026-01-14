/**
 * @file sampler.cpp
 * @brief サンプラーの設定ユーティリティー
 * @author Natsume Shidara
 * @date 2025/09/18
 */

#include "sampler.h"
#include "direct3d.h"

static ID3D11Device* g_pDevice;
static ID3D11DeviceContext* g_pContext;
static ID3D11SamplerState* g_pSamplerStatePoint = nullptr;
static ID3D11SamplerState* g_pSamplerStateLinear = nullptr;
static ID3D11SamplerState* g_pSamplerStateAnisotropic = nullptr;

void Sampler_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    g_pDevice = pDevice;
    g_pContext = pContext;

    // サンプラーステート設定
    D3D11_SAMPLER_DESC sampler_desc{};

    // UV参照外の取り扱い（UVアドレッシングモード）
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0;
    sampler_desc.MaxAnisotropy = 16;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

    // フィルタリング
    sampler_desc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerStatePoint);

    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerStateLinear);

    sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
    g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerStateAnisotropic);
}

void Sampler_Finalize()
{
    SAFE_RELEASE(g_pSamplerStatePoint)
    SAFE_RELEASE(g_pSamplerStateLinear)
    SAFE_RELEASE(g_pSamplerStateAnisotropic)
}

void Sampler_SetFilterPoint()
{
    g_pContext->PSSetSamplers(0, 1, &g_pSamplerStatePoint);
}

void Sampler_SetFilterLinear()
{
    g_pContext->PSSetSamplers(0, 1, &g_pSamplerStateLinear);
}

void Sampler_SetFilterAnisotropic()
{
    g_pContext->PSSetSamplers(0, 1, &g_pSamplerStateAnisotropic);
}
