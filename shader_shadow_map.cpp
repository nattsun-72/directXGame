/****************************************
 * @file shader_shadow_map.cpp
 * @brief シャドウマップ生成用シェーダーの実装
 * @author Natsume Shidara
 * @date 2025/12/10
 ****************************************/

#include "shader_shadow_map.h"
#include <d3dcompiler.h> // ランタイムコンパイル用
#include <string>
#include <sstream>
#include <vector>

 // ライブラリリンク
#pragma comment(lib, "d3dcompiler.lib")

#include "debug_ostream.h"

// セーフリリースマクロ
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) if(x){ x->Release(); x = nullptr; }
#endif

//=============================================================================
// 内部構造体・変数
//=============================================================================

// 定数バッファ構造体 (HLSLの cbuffer ConstantBuffer : register(b0) に対応)
struct CB_ShadowMap
{
    DirectX::XMFLOAT4X4 World;         // ワールド行列
    DirectX::XMFLOAT4X4 LightViewProj; // ライトのビュー * プロジェクション行列
};

// グローバルリソース
static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11PixelShader* g_pPixelShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11Buffer* g_pConstantBuffer = nullptr;

// CPU側のキャッシュ（World更新時にLightViewProjも一緒に送る必要があるため）
static CB_ShadowMap         g_CBData = {};

// デバイス参照
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

//=============================================================================
// 内部ヘルパー関数
//=============================================================================

// HRESULTログ出力
static void LogHR(const char* place, HRESULT hr)
{
    std::ostringstream ss;
    ss << "[ShaderShadowMap] " << place << " failed hr=0x" << std::hex << std::uppercase << hr;
    hal::dout << ss.str() << std::endl;
}

// シェーダーコンパイルヘルパー
static HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined( DEBUG ) || defined( _DEBUG )
    dwShaderFlags |= D3DCOMPILE_DEBUG;
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = nullptr;

    // ファイルからコンパイル
    hr = D3DCompileFromFile(szFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel,
                            dwShaderFlags, 0, ppBlobOut, &pErrorBlob);

    if (FAILED(hr))
    {
        if (pErrorBlob)
        {
            hal::dout << reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()) << std::endl;
            pErrorBlob->Release();
        }
        return hr;
    }

    if (pErrorBlob) pErrorBlob->Release();
    return S_OK;
}

//=============================================================================
// 初期化・終了
//=============================================================================

bool ShaderShadowMap_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    hal::dout << "ShaderShadowMap_Initialize: begin" << std::endl;

    if (!pDevice || !pContext) return false;

    g_pDevice = pDevice;
    g_pContext = pContext;

    HRESULT hr = S_OK;
    ID3DBlob* pVSBlob = nullptr;
    ID3DBlob* pPSBlob = nullptr;
    const WCHAR* hlslPath = L"assets/shader/shadow_map.hlsl"; // プロジェクトルートにある想定

    //----------------------------------------------------------
    // 1. 頂点シェーダー コンパイル & 作成
    //----------------------------------------------------------
    hr = CompileShaderFromFile(hlslPath, "VS_Main", "vs_5_0", &pVSBlob);
    if (FAILED(hr))
    {
        LogHR("Compile VS", hr);
        MessageBox(nullptr, "shadow_map.hlsl のコンパイル(VS)に失敗しました", "エラー", MB_OK);
        return false;
    }

    hr = g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    if (FAILED(hr))
    {
        LogHR("CreateVertexShader", hr);
        pVSBlob->Release();
        return false;
    }

    //----------------------------------------------------------
    // 2. 入力レイアウト作成
    //----------------------------------------------------------
    // 標準的な3Dモデルの頂点構造 (Pos, Normal, Color, UV) を想定し、
    // シェーダーが使わない Color (16byte) をスキップするようにオフセットを指定する。
    // Vertex3D struct: [Pos:0-11] [Norm:12-23] [Color:24-39] [UV:40-47]
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        // COLOR はスキップ
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = g_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pInputLayout);
    pVSBlob->Release(); // レイアウト作成後は不要

    if (FAILED(hr))
    {
        LogHR("CreateInputLayout", hr);
        return false;
    }

    //----------------------------------------------------------
    // 3. ピクセルシェーダー コンパイル & 作成
    //----------------------------------------------------------
    hr = CompileShaderFromFile(hlslPath, "PS_Main", "ps_5_0", &pPSBlob);
    if (FAILED(hr))
    {
        // PSは空でもコンパイルを通す必要がある（エラーなら報告）
        LogHR("Compile PS", hr);
        MessageBox(nullptr, "shadow_map.hlsl のコンパイル(PS)に失敗しました", "エラー", MB_OK);
        return false;
    }

    hr = g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
    pPSBlob->Release();

    if (FAILED(hr))
    {
        LogHR("CreatePixelShader", hr);
        return false;
    }

    //----------------------------------------------------------
    // 4. 定数バッファ作成
    //----------------------------------------------------------
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(CB_ShadowMap); // 128byte (16の倍数)
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = 0;

    hr = g_pDevice->CreateBuffer(&cbDesc, nullptr, &g_pConstantBuffer);
    if (FAILED(hr))
    {
        LogHR("CreateConstantBuffer", hr);
        return false;
    }

    // 行列初期化（単位行列）
    DirectX::XMStoreFloat4x4(&g_CBData.World, DirectX::XMMatrixIdentity());
    DirectX::XMStoreFloat4x4(&g_CBData.LightViewProj, DirectX::XMMatrixIdentity());

    hal::dout << "ShaderShadowMap_Initialize: success" << std::endl;
    return true;
}

void ShaderShadowMap_Finalize()
{
    SAFE_RELEASE(g_pConstantBuffer);
    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pVertexShader);

    g_pDevice = nullptr;
    g_pContext = nullptr;
}

//=============================================================================
// 操作関数
//=============================================================================

void ShaderShadowMap_Begin()
{
    if (!g_pContext) return;

    // シェーダー設定
    g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

    // レイアウト設定
    g_pContext->IASetInputLayout(g_pInputLayout);

    // 定数バッファ設定 (VS b0)
    g_pContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
    // PSは処理がないので設定不要だが、念のため設定しても無害
}

void ShaderShadowMap_SetLightViewProjection(const DirectX::XMFLOAT4X4& matrix)
{
    // キャッシュを更新
    g_CBData.LightViewProj = matrix;

    // ※ ここではGPU更新しない（World更新時にまとめて行う）
}

void ShaderShadowMap_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
    if (!g_pContext || !g_pConstantBuffer) return;

    // ワールド行列を転置して格納
    DirectX::XMStoreFloat4x4(&g_CBData.World, DirectX::XMMatrixTranspose(matrix));

    // CPUキャッシュにある LightViewProj を転置して送る
    CB_ShadowMap sendData;
    sendData.World = g_CBData.World; // 既に転置済み

    // LightViewProjをロードして転置
    DirectX::XMMATRIX lvp = DirectX::XMLoadFloat4x4(&g_CBData.LightViewProj);
    DirectX::XMStoreFloat4x4(&sendData.LightViewProj, DirectX::XMMatrixTranspose(lvp));

    // GPU更新
    g_pContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &sendData, 0, 0);
}