/**
 * @file shader_billboard.cpp
 * @brief シェーダー(ピクセルシェーダーに合わせた修正版)
 * @author Natsume Shidara
 * @date 2025/11/14
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
#include "shader_billboard.h"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x)                                                                                                                                                                                                                                                                                  \
    if ((x) != nullptr)                                                                                                                                                                                                                                                                                  \
    {                                                                                                                                                                                                                                                                                                    \
        (x)->Release();                                                                                                                                                                                                                                                                                  \
        (x) = nullptr;                                                                                                                                                                                                                                                                                   \
    }
#endif

// グローバルリソース
static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11PixelShader* g_pPixelShader = nullptr;

// 定数バッファー（VS用3個: Projection, World, View / PS用1個: Color）
static ID3D11Buffer* g_pVSConstantBuffer0 = nullptr; // Projection Matrix
static ID3D11Buffer* g_pVSConstantBuffer3 = nullptr; // USカット用
static ID3D11Buffer* g_pPSConstantBuffer0 = nullptr; // Material Color (XMFLOAT4)

// デバイス & コンテキスト（外部設定、Release不要）
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;



// ヘルパ: hr をログに出す（16進）
static void LogHR(const char* place, HRESULT hr)
{
    std::ostringstream ss;
    ss << place << " failed hr=0x" << std::hex << std::uppercase << hr;
    hal::dout << ss.str() << std::endl;
}

// ヘルパ: ファイルを読み込んで blob を返す（失敗時は空）
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

// ヘルパ: コンスタントバッファ作成（ByteWidth は 16 バイト境界であること）
static bool CreateConstantBuffer(ID3D11Device* device, UINT byteWidth, ID3D11Buffer** outBuffer)
{
    if (!device || !outBuffer)
        return false;

    D3D11_BUFFER_DESC desc{};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    // D3D11 requires constant buffer size to be multiple of 16
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

bool Shader_Billboard_Initialize()
{
    hal::dout << "Shader_Billboard_Initialize: begin" << std::endl;

    if (!Direct3D_GetDevice() || !Direct3D_GetContext())
    {
        hal::dout << "Shader_Billboard_Initialize: invalid device/context" << std::endl;
        return false;
    }

    g_pDevice = Direct3D_GetDevice();
    g_pContext = Direct3D_GetContext();

    HRESULT hr = S_OK;

    // -----------------------------
    // 頂点シェーダー読み込み & 作成
    // -----------------------------
    const std::string vsPath = "assets/shader/shader_vertex_billboard.cso";
    std::vector<char> vsBlob = LoadFileToBlob(vsPath);
    if (vsBlob.empty())
    {
        MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_billboard.cso", "エラー", MB_OK);
        return false;
    }

    hr = g_pDevice->CreateVertexShader(vsBlob.data(), vsBlob.size(), nullptr, &g_pVertexShader);
    if (FAILED(hr))
    {
        LogHR("CreateVertexShader", hr);
        MessageBox(nullptr, "頂点シェーダーの作成に失敗しました（CreateVertexShader）", "エラー", MB_OK);
        return false;
    }

    // -----------------------------
    // 入力レイアウト作成
    // ※ 頂点シェーダーの入力: POSITION(float3), COLOR(float4), TEXCOORD(float2)
    // -----------------------------
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = g_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob.data(), vsBlob.size(), &g_pInputLayout);
    if (FAILED(hr))
    {
        LogHR("CreateInputLayout", hr);
        MessageBox(nullptr, "頂点レイアウトの作成に失敗しました（CreateInputLayout）。頂点シェーダの入力とC++側のレイアウトを確認してください。", "エラー", MB_OK);
        SAFE_RELEASE(g_pVertexShader);
        return false;
    }

    // -----------------------------
    // 定数バッファ作成（VS：4x4 行列 ×3 / PS：float4 ×1）
    // -----------------------------
    if (!CreateConstantBuffer(g_pDevice, sizeof(XMFLOAT4X4), &g_pVSConstantBuffer0))
    {
        MessageBox(nullptr, "定数バッファ0(Projection)の作成に失敗", "エラー", MB_OK);
        SAFE_RELEASE(g_pInputLayout);
        SAFE_RELEASE(g_pVertexShader);
        return false;
    }
    
    if (!CreateConstantBuffer(g_pDevice, sizeof(XMFLOAT4X4), &g_pVSConstantBuffer3))
    {
        MessageBox(nullptr, "定数バッファ2(View)の作成に失敗", "エラー", MB_OK);
        SAFE_RELEASE(g_pVSConstantBuffer0);
        SAFE_RELEASE(g_pInputLayout);
        SAFE_RELEASE(g_pVertexShader);
        return false;
    }

    if (!CreateConstantBuffer(g_pDevice, sizeof(UVParameter), &g_pPSConstantBuffer0))
    {
        MessageBox(nullptr, "定数バッファ(PS Color)の作成に失敗", "エラー", MB_OK);
        SAFE_RELEASE(g_pVSConstantBuffer0);
        SAFE_RELEASE(g_pInputLayout);
        SAFE_RELEASE(g_pVertexShader);
        return false;
    }

    // -----------------------------
    // ピクセルシェーダー読み込み & 作成
    // -----------------------------
    const std::string psPath = "assets/shader/shader_pixel_billboard.cso";
    std::vector<char> psBlob = LoadFileToBlob(psPath);
    if (psBlob.empty())
    {
        MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_billboard.cso", "エラー", MB_OK);
        SAFE_RELEASE(g_pPSConstantBuffer0);
        SAFE_RELEASE(g_pVSConstantBuffer0);
        SAFE_RELEASE(g_pInputLayout);
        SAFE_RELEASE(g_pVertexShader);
        return false;
    }

    hr = g_pDevice->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, &g_pPixelShader);
    if (FAILED(hr))
    {
        LogHR("CreatePixelShader", hr);
        MessageBox(nullptr, "ピクセルシェーダーの作成に失敗しました（CreatePixelShader）", "エラー", MB_OK);
        SAFE_RELEASE(g_pPSConstantBuffer0);
        SAFE_RELEASE(g_pVSConstantBuffer0);
        SAFE_RELEASE(g_pInputLayout);
        SAFE_RELEASE(g_pVertexShader);
        return false;
    }

    // Sampler は外部の Sampler_SetFilterPoint が担当
    Sampler_SetFilterPoint();

    hal::dout << "Shader_Billboard_Initialize: success" << std::endl;
    return true;
}

void Shader_Billboard_Finalize()
{
    hal::dout << "Shader_Billboard_Finalize: releasing resources" << std::endl;
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pVSConstantBuffer0);
    SAFE_RELEASE(g_pPSConstantBuffer0);
    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pVertexShader);

    // デバイス/コンテキストの参照は外部所有なので解放しないが安全のためポインタをNULLにする
    g_pDevice = nullptr;
    g_pContext = nullptr;
}

void Shader_Billboard_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
    XMFLOAT4X4 transpose;
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));
    g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}


void Shader_Billboard_SetColor(const XMFLOAT4& color)
{
    if (!g_pContext || !g_pPSConstantBuffer0)
        return;

    // マテリアルカラーを定数バッファに設定
    g_pContext->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
}

void Shader_Billboard_SetUVParameter(const UVParameter& parameter) { 
    Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer3, 0, nullptr, &parameter, 0, 0); 
}

void Shader_Billboard_Begin()
{
    if (!g_pContext)
        return;

    // シェーダーをセット
    g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

    // 入力レイアウトをセット
    g_pContext->IASetInputLayout(g_pInputLayout);

    // 定数バッファをバインド（VS用）
    g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);
    g_pContext->VSSetConstantBuffers(3, 1, &g_pVSConstantBuffer3);

    // 定数バッファをバインド（PS用）
    g_pContext->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);

    // サンプラ設定
    Sampler_SetFilterLinear();
}
