/****************************************
 * @file post_process.cpp
 * @brief ポストプロセスエフェクト管理
 * @detail ラジアルブラー、モーションブラー等の画面効果
 * @date 2026/01/10
 * @update シェーダーを.hlslファイルに分離、.cso読み込み方式に変更
 ****************************************/

#include "post_process.h"
#include "direct3d.h"

#include <fstream>
#include <vector>
#include <algorithm>

using namespace DirectX;

//======================================
// マクロ定義
//======================================
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) if(x){ x->Release(); x = nullptr; }
#endif

//======================================
// 定数バッファ構造体
//======================================
struct PostProcessCB
{
    XMFLOAT2 center;        // ブラー中心（UV座標）
    float intensity;        // ブラー強度
    float sampleCount;      // サンプル数
    XMFLOAT2 direction;     // 方向性ブラー用
    float padding[2];
};

//======================================
// 内部変数
//======================================
static ID3D11Texture2D* g_pSceneTexture = nullptr;
static ID3D11RenderTargetView* g_pSceneRTV = nullptr;
static ID3D11ShaderResourceView* g_pSceneSRV = nullptr;

static ID3D11VertexShader* g_pFullscreenVS = nullptr;
static ID3D11PixelShader* g_pRadialBlurPS = nullptr;
static ID3D11PixelShader* g_pDirectionalBlurPS = nullptr;
static ID3D11PixelShader* g_pPassthroughPS = nullptr;

static ID3D11Buffer* g_pConstantBuffer = nullptr;
static ID3D11SamplerState* g_pSamplerState = nullptr;

// エフェクト状態
static PostProcessEffect g_CurrentEffect = PostProcessEffect::None;
static float g_Intensity = 0.0f;
static float g_Duration = 0.0f;
static float g_Timer = 0.0f;
static bool g_FadeOut = true;
static XMFLOAT2 g_BlurDirection = { 0.0f, 0.0f };
static float g_InitialIntensity = 0.0f;

// 元のレンダーターゲット保存用
static ID3D11RenderTargetView* g_pOriginalRTV = nullptr;
static ID3D11DepthStencilView* g_pOriginalDSV = nullptr;

// デバイス参照
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

//======================================
// ヘルパー関数：ファイル読み込み
//======================================
static std::vector<char> LoadFileToBlob(const std::string& path)
{
    std::vector<char> blob;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs)
    {
        OutputDebugStringA(("PostProcess: file open failed: " + path + "\n").c_str());
        return blob;
    }

    ifs.seekg(0, std::ios::end);
    std::streamoff soff = ifs.tellg();
    if (soff <= 0)
    {
        OutputDebugStringA(("PostProcess: file size invalid: " + path + "\n").c_str());
        return blob;
    }

    size_t filesize = static_cast<size_t>(soff);
    ifs.seekg(0, std::ios::beg);
    blob.resize(filesize);

    ifs.read(blob.data(), static_cast<std::streamsize>(filesize));
    if (!ifs)
    {
        OutputDebugStringA(("PostProcess: read failed: " + path + "\n").c_str());
        blob.clear();
        return blob;
    }

    return blob;
}

//======================================
// 初期化
//======================================
void PostProcess_Initialize()
{
    g_pDevice = Direct3D_GetDevice();
    g_pContext = Direct3D_GetContext();

    if (!g_pDevice || !g_pContext)
    {
        OutputDebugStringA("PostProcess_Initialize: invalid device/context\n");
        return;
    }

    UINT width = Direct3D_GetBackBufferWidth();
    UINT height = Direct3D_GetBackBufferHeight();

    HRESULT hr = S_OK;

    //--------------------------------------
    // シーンテクスチャ作成
    //--------------------------------------
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    hr = g_pDevice->CreateTexture2D(&texDesc, nullptr, &g_pSceneTexture);
    if (FAILED(hr))
    {
        OutputDebugStringA("PostProcess: CreateTexture2D failed\n");
        return;
    }

    hr = g_pDevice->CreateRenderTargetView(g_pSceneTexture, nullptr, &g_pSceneRTV);
    if (FAILED(hr))
    {
        OutputDebugStringA("PostProcess: CreateRenderTargetView failed\n");
        PostProcess_Finalize();
        return;
    }

    hr = g_pDevice->CreateShaderResourceView(g_pSceneTexture, nullptr, &g_pSceneSRV);
    if (FAILED(hr))
    {
        OutputDebugStringA("PostProcess: CreateShaderResourceView failed\n");
        PostProcess_Finalize();
        return;
    }

    //--------------------------------------
    // 頂点シェーダー読み込み
    //--------------------------------------
    {
        const std::string vsPath = "assets/shader/shader_postprocess_vs.cso";
        std::vector<char> vsBlob = LoadFileToBlob(vsPath);
        if (vsBlob.empty())
        {
            OutputDebugStringA("PostProcess: VS load failed\n");
            PostProcess_Finalize();
            return;
        }

        hr = g_pDevice->CreateVertexShader(vsBlob.data(), vsBlob.size(), nullptr, &g_pFullscreenVS);
        if (FAILED(hr))
        {
            OutputDebugStringA("PostProcess: CreateVertexShader failed\n");
            PostProcess_Finalize();
            return;
        }
    }

    //--------------------------------------
    // ラジアルブラー ピクセルシェーダー読み込み
    //--------------------------------------
    {
        const std::string psPath = "assets/shader/shader_postprocess_radial_ps.cso";
        std::vector<char> psBlob = LoadFileToBlob(psPath);
        if (psBlob.empty())
        {
            OutputDebugStringA("PostProcess: Radial PS load failed\n");
            PostProcess_Finalize();
            return;
        }

        hr = g_pDevice->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, &g_pRadialBlurPS);
        if (FAILED(hr))
        {
            OutputDebugStringA("PostProcess: CreatePixelShader (Radial) failed\n");
            PostProcess_Finalize();
            return;
        }
    }

    //--------------------------------------
    // 方向性ブラー ピクセルシェーダー読み込み
    //--------------------------------------
    {
        const std::string psPath = "assets/shader/shader_postprocess_directional_ps.cso";
        std::vector<char> psBlob = LoadFileToBlob(psPath);
        if (psBlob.empty())
        {
            OutputDebugStringA("PostProcess: Directional PS load failed\n");
            PostProcess_Finalize();
            return;
        }

        hr = g_pDevice->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, &g_pDirectionalBlurPS);
        if (FAILED(hr))
        {
            OutputDebugStringA("PostProcess: CreatePixelShader (Directional) failed\n");
            PostProcess_Finalize();
            return;
        }
    }

    //--------------------------------------
    // パススルー ピクセルシェーダー読み込み
    //--------------------------------------
    {
        const std::string psPath = "assets/shader/shader_postprocess_passthrough_ps.cso";
        std::vector<char> psBlob = LoadFileToBlob(psPath);
        if (psBlob.empty())
        {
            OutputDebugStringA("PostProcess: Passthrough PS load failed\n");
            PostProcess_Finalize();
            return;
        }

        hr = g_pDevice->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, &g_pPassthroughPS);
        if (FAILED(hr))
        {
            OutputDebugStringA("PostProcess: CreatePixelShader (Passthrough) failed\n");
            PostProcess_Finalize();
            return;
        }
    }

    //--------------------------------------
    // 定数バッファ作成
    //--------------------------------------
    {
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.ByteWidth = sizeof(PostProcessCB);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = g_pDevice->CreateBuffer(&cbDesc, nullptr, &g_pConstantBuffer);
        if (FAILED(hr))
        {
            OutputDebugStringA("PostProcess: CreateBuffer (CB) failed\n");
            PostProcess_Finalize();
            return;
        }
    }

    //--------------------------------------
    // サンプラーステート作成
    //--------------------------------------
    {
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

        hr = g_pDevice->CreateSamplerState(&sampDesc, &g_pSamplerState);
        if (FAILED(hr))
        {
            OutputDebugStringA("PostProcess: CreateSamplerState failed\n");
            PostProcess_Finalize();
            return;
        }
    }

    // 初期状態
    g_CurrentEffect = PostProcessEffect::None;
    g_Intensity = 0.0f;
    g_Timer = 0.0f;

    OutputDebugStringA("PostProcess_Initialize: success\n");
}

//======================================
// 終了処理
//======================================
void PostProcess_Finalize()
{
    SAFE_RELEASE(g_pSamplerState);
    SAFE_RELEASE(g_pConstantBuffer);
    SAFE_RELEASE(g_pPassthroughPS);
    SAFE_RELEASE(g_pDirectionalBlurPS);
    SAFE_RELEASE(g_pRadialBlurPS);
    SAFE_RELEASE(g_pFullscreenVS);
    SAFE_RELEASE(g_pSceneSRV);
    SAFE_RELEASE(g_pSceneRTV);
    SAFE_RELEASE(g_pSceneTexture);

    g_pDevice = nullptr;
    g_pContext = nullptr;

    OutputDebugStringA("PostProcess_Finalize: released\n");
}

//======================================
// シーン描画開始
//======================================
void PostProcess_BeginScene()
{
    if (!g_pContext || !g_pSceneRTV) return;

    // 現在のレンダーターゲットを保存
    g_pContext->OMGetRenderTargets(1, &g_pOriginalRTV, &g_pOriginalDSV);

    // オフスクリーンテクスチャにレンダリング先を切り替え
    g_pContext->OMSetRenderTargets(1, &g_pSceneRTV, g_pOriginalDSV);

    // クリア
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_pContext->ClearRenderTargetView(g_pSceneRTV, clearColor);
}

//======================================
// シーン描画終了 & ポストプロセス適用
//======================================
void PostProcess_EndScene()
{
    if (!g_pContext || !g_pOriginalRTV) return;

    // 元のレンダーターゲット（バックバッファ）に戻す
    g_pContext->OMSetRenderTargets(1, &g_pOriginalRTV, nullptr);

    // 深度テスト無効化
    Direct3D_DepthStencilStateDepthIsEnable(false);

    // シェーダー設定
    g_pContext->VSSetShader(g_pFullscreenVS, nullptr, 0);
    g_pContext->IASetInputLayout(nullptr);
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // テクスチャ設定
    g_pContext->PSSetShaderResources(0, 1, &g_pSceneSRV);
    g_pContext->PSSetSamplers(0, 1, &g_pSamplerState);

    // エフェクトに応じたシェーダーを選択
    ID3D11PixelShader* pPS = g_pPassthroughPS;

    if (g_CurrentEffect == PostProcessEffect::RadialBlur && g_Intensity > 0.001f)
    {
        pPS = g_pRadialBlurPS;

        // 定数バッファ更新
        D3D11_MAPPED_SUBRESOURCE mapped;
        g_pContext->Map(g_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        PostProcessCB* cb = (PostProcessCB*)mapped.pData;
        cb->center = { 0.5f, 0.5f };
        cb->intensity = g_Intensity;
        cb->sampleCount = 16.0f;
        cb->direction = { 0.0f, 0.0f };
        g_pContext->Unmap(g_pConstantBuffer, 0);

        g_pContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
    }
    else if (g_CurrentEffect == PostProcessEffect::DirectionalBlur && g_Intensity > 0.001f)
    {
        pPS = g_pDirectionalBlurPS;

        D3D11_MAPPED_SUBRESOURCE mapped;
        g_pContext->Map(g_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        PostProcessCB* cb = (PostProcessCB*)mapped.pData;
        cb->center = { 0.5f, 0.5f };
        cb->intensity = g_Intensity;
        cb->sampleCount = 12.0f;
        cb->direction = g_BlurDirection;
        g_pContext->Unmap(g_pConstantBuffer, 0);

        g_pContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
    }

    g_pContext->PSSetShader(pPS, nullptr, 0);

    // フルスクリーン三角形描画（頂点バッファ不要）
    g_pContext->Draw(3, 0);

    // シェーダーリソースをクリア（次フレームのレンダリングのため）
    ID3D11ShaderResourceView* nullSRV = nullptr;
    g_pContext->PSSetShaderResources(0, 1, &nullSRV);

    // 保存していたリソースを解放
    SAFE_RELEASE(g_pOriginalRTV);
    SAFE_RELEASE(g_pOriginalDSV);

    // 深度テスト有効化
    Direct3D_DepthStencilStateDepthIsEnable(true);
}

//======================================
// エフェクト制御
//======================================
void PostProcess_StartRadialBlur(float intensity, float duration, bool fadeOut)
{
    g_CurrentEffect = PostProcessEffect::RadialBlur;
    g_Intensity = intensity;
    g_InitialIntensity = intensity;
    g_Duration = duration;
    g_Timer = 0.0f;
    g_FadeOut = fadeOut;
}

void PostProcess_StartDirectionalBlur(const XMFLOAT2& direction, float intensity, float duration)
{
    g_CurrentEffect = PostProcessEffect::DirectionalBlur;
    g_Intensity = intensity;
    g_InitialIntensity = intensity;
    g_Duration = duration;
    g_Timer = 0.0f;
    g_FadeOut = true;

    // 方向を正規化
    XMVECTOR vDir = XMLoadFloat2(&direction);
    vDir = XMVector2Normalize(vDir);
    XMStoreFloat2(&g_BlurDirection, vDir);
}

void PostProcess_Stop()
{
    g_CurrentEffect = PostProcessEffect::None;
    g_Intensity = 0.0f;
    g_Timer = 0.0f;
}

void PostProcess_Update(float dt)
{
    if (g_CurrentEffect == PostProcessEffect::None) return;

    g_Timer += dt;

    if (g_Timer >= g_Duration)
    {
        // エフェクト終了
        g_CurrentEffect = PostProcessEffect::None;
        g_Intensity = 0.0f;
        return;
    }

    // フェードアウト処理
    if (g_FadeOut)
    {
        float t = g_Timer / g_Duration;
        // イージング（最初は強く、徐々に弱く）
        float easedT = 1.0f - (1.0f - t) * (1.0f - t);
        g_Intensity = g_InitialIntensity * (1.0f - easedT);
    }
}

//======================================
// 状態取得
//======================================
bool PostProcess_IsActive()
{
    return g_CurrentEffect != PostProcessEffect::None && g_Intensity > 0.001f;
}

PostProcessEffect PostProcess_GetCurrentEffect()
{
    return g_CurrentEffect;
}

float PostProcess_GetCurrentIntensity()
{
    return g_Intensity;
}