/****************************************
 * @file render_target.cpp
 * @brief オフスクリーンレンダリング用ターゲット管理の実装
 * @author Natsume Shidara
 * @date 2025/12/10
 * @update 2025/12/10
 ****************************************/

#include "render_target.h"
#include "direct3d.h"       // Direct3D_GetDevice(), Direct3D_GetContext()
#include "debug_ostream.h"

 // コンストラクタ
RenderTarget::RenderTarget()
    : m_pTexture(nullptr)
    , m_pRTV(nullptr)
    , m_pDSV(nullptr)
    , m_pSRV(nullptr)
    , m_pDepthTexture(nullptr)
    , m_Viewport{}
    , m_Type(RenderTargetType::Color)
{
    // デフォルトクリア色（透明な黒）
    m_ClearColor[0] = 0.0f;
    m_ClearColor[1] = 0.0f;
    m_ClearColor[2] = 0.0f;
    m_ClearColor[3] = 0.0f;
}

// デストラクタ
RenderTarget::~RenderTarget()
{
    Finalize();
}

// 初期化
bool RenderTarget::Initialize(unsigned int width, unsigned int height, RenderTargetType type)
{
    HRESULT hr;
    ID3D11Device* pDevice = Direct3D_GetDevice();
    if (!pDevice) return false;

    m_Type = type;

    // ビューポート設定（共通）
    m_Viewport.TopLeftX = 0.0f;
    m_Viewport.TopLeftY = 0.0f;
    m_Viewport.Width = static_cast<float>(width);
    m_Viewport.Height = static_cast<float>(height);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    //----------------------------------------------------------
    // タイプ別のリソース生成
    //----------------------------------------------------------
    if (m_Type == RenderTargetType::Color)
    {
        //======================================================
        // Colorモード（ミニマップ用など）
        // 色用テクスチャ(RTV/SRV) ＋ 深度バッファ(DSV) を作成
        //======================================================

        // 1. カラーテクスチャ作成
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 一般的なカラー
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        hr = pDevice->CreateTexture2D(&texDesc, nullptr, &m_pTexture);
        if (FAILED(hr)) return false;

        // RTV作成
        hr = pDevice->CreateRenderTargetView(m_pTexture, nullptr, &m_pRTV);
        if (FAILED(hr)) return false;

        // SRV作成（色を参照）
        hr = pDevice->CreateShaderResourceView(m_pTexture, nullptr, &m_pSRV);
        if (FAILED(hr)) return false;

        // 2. 深度バッファ作成（描画時の前後関係判定用、読み込みはしない）
        D3D11_TEXTURE2D_DESC depthDesc = texDesc;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        hr = pDevice->CreateTexture2D(&depthDesc, nullptr, &m_pDepthTexture);
        if (FAILED(hr)) return false;

        // DSV作成
        hr = pDevice->CreateDepthStencilView(m_pDepthTexture, nullptr, &m_pDSV);
        if (FAILED(hr)) return false;
    }
    else // RenderTargetType::Shadow
    {
        //======================================================
        // Shadowモード（シャドウマップ用）
        // 深度そのものをテクスチャ(SRV)として扱うための特殊設定
        //======================================================

        // 1. 深度テクスチャ作成（TYPELESSフォーマットが必須）
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R32_TYPELESS; // 重要：用途未定の32bit
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        // 深度バッファとしても、テクスチャとしても使う
        texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

        hr = pDevice->CreateTexture2D(&texDesc, nullptr, &m_pTexture);
        if (FAILED(hr)) return false;

        // DSV作成（書き込み時は D32_FLOAT として扱う）
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;

        hr = pDevice->CreateDepthStencilView(m_pTexture, &dsvDesc, &m_pDSV);
        if (FAILED(hr)) return false;

        // SRV作成（読み込み時は R32_FLOAT として扱う）
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        hr = pDevice->CreateShaderResourceView(m_pTexture, &srvDesc, &m_pSRV);
        if (FAILED(hr)) return false;

        // ShadowモードではRTVは不要
        m_pRTV = nullptr;
    }

    return true;
}

// 解放
void RenderTarget::Finalize()
{
    SAFE_RELEASE(m_pSRV);
    SAFE_RELEASE(m_pDSV);
    SAFE_RELEASE(m_pRTV);
    SAFE_RELEASE(m_pTexture);
    SAFE_RELEASE(m_pDepthTexture);
}

// 描画先のアクティブ化
void RenderTarget::Activate()
{
    ID3D11DeviceContext* pContext = Direct3D_GetContext();
    if (!pContext) return;

    // 1. ビューポート設定
    pContext->RSSetViewports(1, &m_Viewport);

    // 2. レンダーターゲット設定 & クリア処理
    if (m_Type == RenderTargetType::Color)
    {
        // Colorモード: RTVとDSVをセットし、両方クリア
        pContext->OMSetRenderTargets(1, &m_pRTV, m_pDSV);
        pContext->ClearRenderTargetView(m_pRTV, m_ClearColor);
        pContext->ClearDepthStencilView(m_pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }
    else
    {
        // Shadowモード: RTVは無し(NULL)で、DSVのみセット。深度のみクリア
        ID3D11RenderTargetView* nullRTV = nullptr;
        pContext->OMSetRenderTargets(1, &nullRTV, m_pDSV);
        pContext->ClearDepthStencilView(m_pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
    }
}

// クリア色設定
void RenderTarget::SetClearColor(float r, float g, float b, float a)
{
    m_ClearColor[0] = r;
    m_ClearColor[1] = g;
    m_ClearColor[2] = b;
    m_ClearColor[3] = a;
}

// SRV取得
ID3D11ShaderResourceView* RenderTarget::GetSRV() const
{
    return m_pSRV;
}