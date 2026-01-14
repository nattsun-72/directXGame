/**
 * @file shader3d.cpp
 * @brief シェーダー（改良版：ShadowMap対応・goto排除・堅牢化）
 * @author Natsume Shidara
 * @date 2025/12/10
 */

#include <DirectXMath.h>
#include <d3d11.h>
#include <windows.h> // MessageBox 等

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace DirectX;

#include "debug_ostream.h"
#include "direct3d.h"
#include "sampler.h"
#include "shader3d.h"

// Project に SAFE_RELEASE マクロがある想定。ただし無ければ以下を使う。
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) if(x){ x->Release(); x = nullptr; }
#endif

//=============================================================================
// グローバルリソース
//=============================================================================
static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11PixelShader* g_pPixelShader = nullptr;

// 定数バッファー
// VS b0: World
static ID3D11Buffer* g_pVSConstantBuffer0 = nullptr;
// VS b3: LightViewProjection (ShadowMap用)
static ID3D11Buffer* g_pVSConstantBuffer_Shadow = nullptr;

// PS b0: Color
static ID3D11Buffer* g_pPSConstantBuffer0 = nullptr;

// デバイス & コンテキスト（外部設定、Release不要）
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

//=============================================================================
// ヘルパー関数
//=============================================================================

// hr をログに出す（16進）
static void LogHR(const char* place, HRESULT hr)
{
    std::ostringstream ss;
    ss << place << " failed hr=0x" << std::hex << std::uppercase << hr;
    hal::dout << ss.str() << std::endl;
}

// ファイルを読み込んで blob を返す（失敗時は空）
static std::vector<char> LoadFileToBlob(const std::string& path)
{
    std::vector<char> blob;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs)
    {
        hal::dout << "LoadFileToBlob: file open failed: " << path << std::endl;
        return blob;
    }

    ifs.seekg(0, std::ios::end);
    std::streamoff soff = ifs.tellg();
    if (soff <= 0)
    {
        hal::dout << "LoadFileToBlob: file size invalid: " << path << std::endl;
        return blob;
    }

    size_t filesize = static_cast<size_t>(soff);
    ifs.seekg(0, std::ios::beg);
    blob.resize(filesize);

    ifs.read(blob.data(), static_cast<std::streamsize>(filesize));
    if (!ifs)
    {
        hal::dout << "LoadFileToBlob: read failed: " << path << std::endl;
        blob.clear();
        return blob;
    }

    return blob;
}

// コンスタントバッファ作成（ByteWidth は 16 バイト境界であること）
static bool CreateConstantBuffer(ID3D11Device* device, UINT byteWidth, ID3D11Buffer** outBuffer)
{
    if (!device || !outBuffer) return false;

    D3D11_BUFFER_DESC desc{};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    // 16バイト境界に調整
    desc.ByteWidth = ((byteWidth + 15) / 16) * 16;

    HRESULT hr = device->CreateBuffer(&desc, nullptr, outBuffer);
    if (FAILED(hr))
    {
        LogHR("CreateConstantBuffer", hr);
        *outBuffer = nullptr;
        return false;
    }
    return true;
}

//=============================================================================
// 初期化・終了
//=============================================================================

bool Shader3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    hal::dout << "Shader3D_Initialize: begin" << std::endl;

    if (!pDevice || !pContext)
    {
        hal::dout << "Shader3D_Initialize: invalid device/context" << std::endl;
        return false;
    }

    g_pDevice = pDevice;
    g_pContext = pContext;

    HRESULT hr = S_OK;

    // -----------------------------------------------------
    // 1. 頂点シェーダー読み込み & 作成
    // -----------------------------------------------------
    {
        const std::string vsPath = "assets/shader/shader_vertex_3d.cso";
        std::vector<char> vsBlob = LoadFileToBlob(vsPath);
        if (vsBlob.empty())
        {
            MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_3d.cso", "エラー", MB_OK);
            Shader3D_Finalize(); // 失敗したら即片付けて終了
            return false;
        }

        hr = g_pDevice->CreateVertexShader(vsBlob.data(), vsBlob.size(), nullptr, &g_pVertexShader);
        if (FAILED(hr))
        {
            LogHR("CreateVertexShader", hr);
            MessageBox(nullptr, "頂点シェーダーの作成に失敗しました", "エラー", MB_OK);
            Shader3D_Finalize();
            return false;
        }

        // -----------------------------------------------------
        // 2. 入力レイアウト作成
        // -----------------------------------------------------
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        hr = g_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob.data(), vsBlob.size(), &g_pInputLayout);
        if (FAILED(hr))
        {
            LogHR("CreateInputLayout", hr);
            MessageBox(nullptr, "入力レイアウトの作成に失敗しました", "エラー", MB_OK);
            Shader3D_Finalize();
            return false;
        }
    } // vsBlob はここで破棄される

    // -----------------------------------------------------
    // 3. 定数バッファ作成
    // -----------------------------------------------------
    // VS b0: World
    if (!CreateConstantBuffer(g_pDevice, sizeof(XMFLOAT4X4), &g_pVSConstantBuffer0))
    {
        Shader3D_Finalize();
        return false;
    }

    // VS b3: LightViewProjection
    if (!CreateConstantBuffer(g_pDevice, sizeof(XMFLOAT4X4), &g_pVSConstantBuffer_Shadow))
    {
        Shader3D_Finalize();
        return false;
    }

    // PS b0: Color
    if (!CreateConstantBuffer(g_pDevice, sizeof(XMFLOAT4), &g_pPSConstantBuffer0))
    {
        Shader3D_Finalize();
        return false;
    }

    // -----------------------------------------------------
    // 4. ピクセルシェーダー読み込み & 作成
    // -----------------------------------------------------
    {
        const std::string psPath = "assets/shader/shader_pixel_3d.cso";
        std::vector<char> psBlob = LoadFileToBlob(psPath);
        if (psBlob.empty())
        {
            MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_3d.cso", "エラー", MB_OK);
            Shader3D_Finalize();
            return false;
        }

        hr = g_pDevice->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, &g_pPixelShader);
        if (FAILED(hr))
        {
            LogHR("CreatePixelShader", hr);
            MessageBox(nullptr, "ピクセルシェーダーの作成に失敗しました", "エラー", MB_OK);
            Shader3D_Finalize();
            return false;
        }
    } // psBlob はここで破棄される

    // -----------------------------------------------------
    // 5. サンプラ設定
    // -----------------------------------------------------
    Sampler_SetFilterPoint();

    hal::dout << "Shader3D_Initialize: success" << std::endl;
    return true;
}

void Shader3D_Finalize()
{
    hal::dout << "Shader3D_Finalize: releasing resources" << std::endl;

    SAFE_RELEASE(g_pPixelShader);

    SAFE_RELEASE(g_pVSConstantBuffer0);
    SAFE_RELEASE(g_pVSConstantBuffer_Shadow);
    SAFE_RELEASE(g_pPSConstantBuffer0);

    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pVertexShader);

    g_pDevice = nullptr;
    g_pContext = nullptr;
}

//=============================================================================
// 操作関数
//=============================================================================

void Shader3D_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
    if (!g_pContext || !g_pVSConstantBuffer0) return;

    XMFLOAT4X4 transpose;
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));
    g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

void Shader3D_SetLightViewProjection(const DirectX::XMFLOAT4X4& matrix)
{
    if (!g_pContext || !g_pVSConstantBuffer_Shadow) return;

    XMMATRIX m = XMLoadFloat4x4(&matrix);
    XMFLOAT4X4 transpose;
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(m));

    g_pContext->UpdateSubresource(g_pVSConstantBuffer_Shadow, 0, nullptr, &transpose, 0, 0);
}

void Shader3D_SetShadowMap(ID3D11ShaderResourceView* pShadowSRV)
{
    if (!g_pContext) return;

    // ピクセルシェーダーのスロット1番に設定
    g_pContext->PSSetShaderResources(1, 1, &pShadowSRV);
}

void Shader3D_SetColor(const XMFLOAT4& color)
{
    if (!g_pContext || !g_pPSConstantBuffer0) return;

    g_pContext->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
}

void Shader3D_Begin()
{
    if (!g_pContext) return;

    // シェーダーをセット
    g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

    // 入力レイアウトをセット
    g_pContext->IASetInputLayout(g_pInputLayout);


    // VS b0: World 行列
    g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);

    // VS b3: LightViewProjection 行列 (ShadowMap用)
    g_pContext->VSSetConstantBuffers(3, 1, &g_pVSConstantBuffer_Shadow);

    // PS b0: Color / Material
    g_pContext->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);

    // サンプラ設定
    Sampler_SetFilterPoint();
}