/**
 * @file shader.cpp
 * @brief シェーダー
 * @author Natsume Shidara
 * @date 2025/06/10
 */

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;
#include "direct3d.h"
#include "sampler.h"
#include "debug_ostream.h"
#include <fstream>


static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11PixelShader* g_pPixelShader = nullptr;

// 定数バッファー
static ID3D11Buffer* g_pVSConstantBuffer0 = nullptr; // Projection Matrix
static ID3D11Buffer* g_pVSConstantBuffer1 = nullptr; // World Matrix

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


bool Shader_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    HRESULT hr; // 戻り値格納用

    // デバイスとデバイスコンテキストのチェック
    if (!pDevice || !pContext)
    {
        hal::dout << "Shader_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
        return false;
    }

    // デバイスとデバイスコンテキストの保存
    g_pDevice = pDevice;
    g_pContext = pContext;


    // 事前コンパイル済み頂点シェーダーの読み込み
    std::ifstream ifs_vs("assets/shader/shader_vertex_2d.cso", std::ios::binary);

    if (!ifs_vs)
    {
        MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_2d.cso", "エラー", MB_OK);
        return false;
    }

    // ファイルサイズを取得
    ifs_vs.seekg(0, std::ios::end); // ファイルポインタを末尾に移動
    std::streamsize filesize = ifs_vs.tellg(); // ファイルポインタの位置を取得（つまりファイルサイズ）
    ifs_vs.seekg(0, std::ios::beg); // ファイルポインタを先頭に戻す

    // バイナリデータを格納するためのバッファを確保
    unsigned char* vsbinary_pointer = new unsigned char[filesize];

    ifs_vs.read((char*)vsbinary_pointer, filesize); // バイナリデータを読み込む
    ifs_vs.close(); // ファイルを閉じる

    // 頂点シェーダーの作成
    hr = g_pDevice->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pVertexShader);

    if (FAILED(hr))
    {
        hal::dout << "Shader_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
        delete[] vsbinary_pointer; // メモリリークしないようにバイナリデータのバッファを解放
        return false;
    }


    // 頂点レイアウトの定義
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    UINT num_elements = ARRAYSIZE(layout); // 配列の要素数を取得

    // 頂点レイアウトの作成
    hr = g_pDevice->CreateInputLayout(layout, num_elements, vsbinary_pointer, filesize, &g_pInputLayout);

    delete[] vsbinary_pointer; // バイナリデータのバッファを解放

    if (FAILED(hr))
    {
        hal::dout << "Shader_Initialize() : 頂点レイアウトの作成に失敗しました" << std::endl;
        return false;
    }


    // 頂点シェーダー用定数バッファの作成
    D3D11_BUFFER_DESC buffer_desc{};
    buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズ
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ

    g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer0);
    g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer1);


    // 事前コンパイル済みピクセルシェーダーの読み込み
    std::ifstream ifs_ps("assets/shader/shader_pixel_2d.cso", std::ios::binary);
    if (!ifs_ps)
    {
        MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_2d.cso", "エラー", MB_OK);
        return false;
    }

    ifs_ps.seekg(0, std::ios::end);
    filesize = ifs_ps.tellg();
    ifs_ps.seekg(0, std::ios::beg);

    unsigned char* psbinary_pointer = new unsigned char[filesize];
    ifs_ps.read((char*)psbinary_pointer, filesize);
    ifs_ps.close();

    // ピクセルシェーダーの作成
    hr = g_pDevice->CreatePixelShader(psbinary_pointer, filesize, nullptr, &g_pPixelShader);

    delete[] psbinary_pointer; // バイナリデータのバッファを解放

    if (FAILED(hr))
    {
        hal::dout << "Shader_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
        return false;
    }
    
    // サンプラーステート設定
    D3D11_SAMPLER_DESC sampler_desc{};

    // フィルタリング
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

    // UV参照外の取り扱い（UVアドレッシングモード）
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0;
    sampler_desc.MaxAnisotropy = 8;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

    return true;
}

void Shader_Finalize()
{
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pVSConstantBuffer0);
    SAFE_RELEASE(g_pVSConstantBuffer1);
    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pVertexShader);
}

void Shader_SetProjectionMatrix(const DirectX::XMMATRIX& matrix)
{
    // 定数バッファ格納用行列の構造体を定義
    XMFLOAT4X4 transpose;

    // 行列を転置して定数バッファ格納用行列に変換
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

    // 定数バッファに行列をセット
    g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

void Shader_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
    XMFLOAT4X4 transpose;

    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

    g_pContext->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &transpose, 0, 0);
}

void Shader_Begin()
{
    // 頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
    g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

    // 頂点レイアウトを描画パイプラインに設定
    g_pContext->IASetInputLayout(g_pInputLayout);

    // 定数バッファを描画パイプラインに設定
    g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);
    g_pContext->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);

    // サンプラーステートを描画パイプラインに設定
    Sampler_SetFilterPoint();
}
