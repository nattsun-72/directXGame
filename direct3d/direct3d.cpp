/****************************************
 * @file direct3d.cpp
 * @brief Direct3Dの初期化および描画関連処理
 * @author Natsume Shidara
 * @date 2025/06/06
 * @update 2026/02/06
 *
 * Direct3D11のデバイス、スワップチェーン、バックバッファの管理を行う。
 * 汎用的なブレンドステート、深度ステート、および
 * シャドウマップ生成に必要なラスタライザーステートの管理も担当する。
 ****************************************/

#include "direct3d.h"
#include "debug_ostream.h"

using namespace DirectX;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

//======================================
// Direct3D 各種インターフェース
//======================================
static ID3D11Device* g_pDevice = nullptr;               // 描画デバイス
static ID3D11DeviceContext* g_pDeviceContext = nullptr; // デバイスコンテキスト
static IDXGISwapChain* g_pSwapChain = nullptr;          // スワップチェーン

//======================================
// ステート関連
//======================================
// ブレンドステート群
static ID3D11BlendState* g_pBlendStateNone = nullptr;     // ブレンドなし
static ID3D11BlendState* g_pBlendStateAlpha = nullptr;    // 通常（アルファブレンド）
static ID3D11BlendState* g_pBlendStateAdd = nullptr;      // 加算合成
static ID3D11BlendState* g_pBlendStateMultiply = nullptr; // 乗算合成

// 深度ステンシルステート群
static ID3D11DepthStencilState* g_pDepthStencilStateDepthDisable = nullptr; // 深度無効ステート
static ID3D11DepthStencilState* g_pDepthStencilStateDepthEnable = nullptr;  // 深度有効ステート
static ID3D11DepthStencilState* g_pDepthWriteDisable = nullptr;             // 深度書き込み無効ステート

// ラスタライザーステート群
static ID3D11RasterizerState* g_pRSDefault = nullptr; // 通常描画用
static ID3D11RasterizerState* g_pRSShadow = nullptr;  // 影生成用（深度バイアスあり）

//======================================
// バックバッファ関連
//======================================
static ID3D11RenderTargetView* g_pRenderTargetView = nullptr; // メインの描画ターゲット
static ID3D11Texture2D* g_pDepthStencilBuffer = nullptr;      // メインの深度バッファ
static ID3D11DepthStencilView* g_pDepthStencilView = nullptr; // メインの深度ビュー
static D3D11_TEXTURE2D_DESC    g_BackBufferDesc{};            // バックバッファ情報

static D3D11_VIEWPORT          g_Viewport{};                  // メインビューポート設定

//======================================
// 内部関数宣言
//======================================
static bool configureBackBuffer(); // バックバッファ生成・設定
static void releaseBackBuffer();   // バックバッファ解放

// ステート作成ヘルパー
static HRESULT createBlendState(ID3D11BlendState** ppBlendState, D3D11_BLEND srcBlend, D3D11_BLEND destBlend, D3D11_BLEND_OP blendOp = D3D11_BLEND_OP_ADD);
static HRESULT createRasterizerState();

//======================================
// 関数群：Direct3D初期化・解放
//======================================

/**
 * @brief Direct3D初期化
 * @param hWnd ウィンドウハンドル
 * @return 初期化成功ならtrue、失敗ならfalse
 */
bool Direct3D_Initialize(HWND hWnd)
{
    // スワップチェーン設定
    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
    swap_chain_desc.Windowed = TRUE;                                    // ウィンドウモード
    swap_chain_desc.BufferCount = 2;                                    // バックバッファ枚数
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // RGBA形式
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // 描画対象
    swap_chain_desc.SampleDesc.Count = 1;                               // マルチサンプリングなし
    swap_chain_desc.SampleDesc.Quality = 0;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;      // 表示方式
    swap_chain_desc.OutputWindow = hWnd;                                // 対象ウィンドウ

    UINT device_flags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    device_flags |= D3D11_CREATE_DEVICE_DEBUG; // デバッグフラグ
#endif

    // サポートするFeatureLevel指定
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

    // デバイスとスワップチェーン生成
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, device_flags, levels, ARRAYSIZE(levels), D3D11_SDK_VERSION, &swap_chain_desc, &g_pSwapChain, &g_pDevice, &feature_level, &g_pDeviceContext);

    if (FAILED(hr))
    {
        MessageBox(hWnd, "Direct3Dの初期化に失敗しました", "エラー", MB_OK);
        return false;
    }

    // バックバッファ設定
    if (!configureBackBuffer())
    {
        MessageBox(hWnd, "バックバッファの設定に失敗しました", "エラー", MB_OK);
        return false;
    }

    //======================================
    // ラスタライザーステート作成
    //======================================
    if (FAILED(createRasterizerState()))
    {
        MessageBox(hWnd, "ラスタライザーステートの作成に失敗しました", "エラー", MB_OK);
        return false;
    }
    Direct3D_SetRasterizerState_Default(); // 初期値はデフォルト

    //======================================
    // ブレンドステート作成
    //======================================
    /*
     * 1. ブレンドなし（不透明）
     * C_out = C_src * 1 + C_dest * 0
     */
    if (FAILED(createBlendState(&g_pBlendStateNone, D3D11_BLEND_ONE, D3D11_BLEND_ZERO))) return false;

    /*
     * 2. 通常合成（アルファブレンド）
     * C_out = C_src * A_src + C_dest * (1 - A_src)
     */
    if (FAILED(createBlendState(&g_pBlendStateAlpha, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA))) return false;

    /*
     * 3. 加算合成（修正案）
     * C_out = C_src * A_src + C_dest * 1
     * もしテクスチャ自体にアルファ乗算済み(Premultiplied Alpha)を使用している場合は
     * D3D11_BLEND_ONE, D3D11_BLEND_ONE の組み合わせが一般的です。
     */
    if (FAILED(createBlendState(&g_pBlendStateAdd, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE))) return false;

    /*
     * 4. 乗算合成
     * C_out = C_src * 0 + C_dest * C_src
     */
    if (FAILED(createBlendState(&g_pBlendStateMultiply, D3D11_BLEND_ZERO, D3D11_BLEND_SRC_COLOR))) return false;

    // 初期値設定：通常アルファブレンド
    Direct3D_SetBlendState(BlendMode::Alpha);

    //======================================
    // 深度ステンシルステート設定
    //======================================
    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.StencilEnable = FALSE;

    // 1. 無効ステート（Zテストなし、Z書き込みなし）
    dsd.DepthEnable = FALSE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsd.DepthFunc = D3D11_COMPARISON_LESS;
    g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthStencilStateDepthDisable);

    // 2. 有効ステート（Zテストあり、Z書き込みあり）
    dsd.DepthEnable = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsd.DepthFunc = D3D11_COMPARISON_LESS;
    g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthStencilStateDepthEnable);

    // 3. 書き込み無効化ステート（Zテストあり、Z書き込みなし）※半透明・加算描画用
    dsd.DepthEnable = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsd.DepthFunc = D3D11_COMPARISON_LESS;
    g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthWriteDisable);

    // デフォルトは有効（3D空間描画を想定）
    Direct3D_DepthStencilStateDepthIsEnable(true);

    return true;
}

/**
 * @brief Direct3D終了処理
 */
void Direct3D_Finalize()
{
    releaseBackBuffer();

    // ステート解放
    SAFE_RELEASE(g_pRSDefault);
    SAFE_RELEASE(g_pRSShadow);

    SAFE_RELEASE(g_pDepthWriteDisable);
    SAFE_RELEASE(g_pDepthStencilStateDepthDisable);
    SAFE_RELEASE(g_pDepthStencilStateDepthEnable);

    SAFE_RELEASE(g_pBlendStateNone);
    SAFE_RELEASE(g_pBlendStateAlpha);
    SAFE_RELEASE(g_pBlendStateAdd);
    SAFE_RELEASE(g_pBlendStateMultiply);

    SAFE_RELEASE(g_pSwapChain);
    SAFE_RELEASE(g_pDeviceContext);
    SAFE_RELEASE(g_pDevice);
}

//======================================
// 関数群：描画処理・ステート変更
//======================================

/**
 * @brief 描画バッファをクリア
 */
void Direct3D_Clear()
{
    float clear_color[4] = { 0.2f, 0.4f, 0.8f, 1.0f }; // 背景色
    g_pDeviceContext->ClearRenderTargetView(g_pRenderTargetView, clear_color);
    g_pDeviceContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
}

/**
 * @brief 描画結果を画面に表示
 */
void Direct3D_Present()
{
    g_pSwapChain->Present(1, 0); // 垂直同期有効
}

/**
 * @brief ブレンドステートの設定
 * @param mode 設定したいブレンドモード
 */
void Direct3D_SetBlendState(BlendMode mode)
{
    ID3D11BlendState* pState = nullptr;
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    switch (mode)
    {
        case BlendMode::None:     pState = g_pBlendStateNone; break;
        case BlendMode::Alpha:    pState = g_pBlendStateAlpha; break;
        case BlendMode::Add:      pState = g_pBlendStateAdd; break;
        case BlendMode::Multiply: pState = g_pBlendStateMultiply; break;
        default:                  pState = g_pBlendStateAlpha; break;
    }

    g_pDeviceContext->OMSetBlendState(pState, blendFactor, 0xFFFFFFFF);
}

/**
 * @brief 影生成用のラスタライザーステート（深度バイアス有効）を設定
 */
void Direct3D_SetRasterizerState_Shadow()
{
    g_pDeviceContext->RSSetState(g_pRSShadow);
}

/**
 * @brief 通常のラスタライザーステートを設定
 */
void Direct3D_SetRasterizerState_Default()
{
    g_pDeviceContext->RSSetState(g_pRSDefault);
}

/**
 * @brief 描画先を通常のバックバッファ（画面）に戻す
 */
void Direct3D_SetBackBufferRenderTarget()
{
    // ビューポートをウィンドウサイズに戻す
    g_pDeviceContext->RSSetViewports(1, &g_Viewport);

    // ターゲットをバックバッファに戻す
    g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
}

/**
 * @brief ビューポート行列の作成
 * @return ビューポート変換行列
 */
DirectX::XMMATRIX Direct3D_MatrixViewPort()
{
    float halfWidth = g_Viewport.Width * 0.5f;
    float halfHeight = g_Viewport.Height * 0.5f;
    float maxDepth = g_Viewport.MaxDepth;
    float minDepth = g_Viewport.MinDepth;
    float topX = g_Viewport.TopLeftX;
    float topY = g_Viewport.TopLeftY;

    return DirectX::XMMATRIX(
        halfWidth, 0.0f, 0.0f, 0.0f,
        0.0f, -halfHeight, 0.0f, 0.0f,
        0.0f, 0.0f, (maxDepth - minDepth), 0.0f,
        halfWidth + topX, halfHeight + topY, minDepth, 1.0f
    );
}

/**
 * @brief スクリーン座標からワールド座標へ逆変換
 */
XMFLOAT3 Direct3D_ScreenToWorld(int x, int y, float depth, const XMFLOAT4X4& view, const XMFLOAT4X4& projection)
{
    XMMATRIX xview = XMLoadFloat4x4(&view);
    XMMATRIX xproj = XMLoadFloat4x4(&projection);

    float centerX = static_cast<float>(x) + 0.5f;
    float centerY = static_cast<float>(y) + 0.5f;

    XMVECTOR xpoint = XMVectorSet(centerX, centerY, depth, 1.0f);

    XMMATRIX matVP = Direct3D_MatrixViewPort();
    XMMATRIX matInv = XMMatrixInverse(nullptr, xview * xproj * matVP);

    xpoint = XMVector3TransformCoord(xpoint, matInv);

    XMFLOAT3 ret;
    XMStoreFloat3(&ret, xpoint);

    return ret;
}

/**
 * @brief ワールド座標からスクリーン座標へ変換
 */
DirectX::XMFLOAT2 Direct3D_WorldToScreen(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
    XMMATRIX xview = XMLoadFloat4x4(&view);
    XMMATRIX xproj = XMLoadFloat4x4(&projection);
    XMVECTOR xpoint = XMLoadFloat3(&position);

    xpoint = XMVector3TransformCoord(xpoint, xview * xproj * Direct3D_MatrixViewPort());

    XMFLOAT2 ret;
    XMStoreFloat2(&ret, xpoint);

    return ret;
}

//======================================
// アクセサ
//======================================
unsigned int Direct3D_GetBackBufferWidth() { return g_BackBufferDesc.Width; }
unsigned int Direct3D_GetBackBufferHeight() { return g_BackBufferDesc.Height; }
ID3D11Device* Direct3D_GetDevice() { return g_pDevice; }
ID3D11DeviceContext* Direct3D_GetContext() { return g_pDeviceContext; }

//======================================
// 内部関数：設定/解放
//======================================

/**
 * @brief バックバッファ生成・設定
 */
bool configureBackBuffer()
{
    HRESULT hr;
    ID3D11Texture2D* back_buffer_pointer = nullptr;

    // バックバッファの取得
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer_pointer);
    if (FAILED(hr))
    {
        hal::dout << "バックバッファの取得に失敗しました" << std::endl;
        return false;
    }

    // レンダーターゲットビュー生成
    hr = g_pDevice->CreateRenderTargetView(back_buffer_pointer, nullptr, &g_pRenderTargetView);
    if (FAILED(hr))
    {
        back_buffer_pointer->Release();
        hal::dout << "レンダーターゲットビュー生成に失敗しました" << std::endl;
        return false;
    }

    back_buffer_pointer->GetDesc(&g_BackBufferDesc); // バックバッファ情報取得
    back_buffer_pointer->Release();

    // デプスステンシルバッファ生成
    D3D11_TEXTURE2D_DESC depth_stencil_desc{};
    depth_stencil_desc.Width = g_BackBufferDesc.Width;
    depth_stencil_desc.Height = g_BackBufferDesc.Height;
    depth_stencil_desc.MipLevels = 1;
    depth_stencil_desc.ArraySize = 1;
    depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_desc.SampleDesc.Count = 1;
    depth_stencil_desc.SampleDesc.Quality = 0;
    depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = g_pDevice->CreateTexture2D(&depth_stencil_desc, nullptr, &g_pDepthStencilBuffer);
    if (FAILED(hr))
    {
        hal::dout << "デプスステンシルバッファ生成に失敗しました" << std::endl;
        return false;
    }

    // デプスステンシルビュー生成
    D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
    depth_stencil_view_desc.Format = depth_stencil_desc.Format;
    depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depth_stencil_view_desc.Texture2D.MipSlice = 0;
    hr = g_pDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &depth_stencil_view_desc, &g_pDepthStencilView);
    if (FAILED(hr))
    {
        hal::dout << "デプスステンシルビュー生成に失敗しました" << std::endl;
        return false;
    }

    // ビューポート設定
    g_Viewport.TopLeftX = 0.0f;
    g_Viewport.TopLeftY = 0.0f;
    g_Viewport.Width = static_cast<FLOAT>(g_BackBufferDesc.Width);
    g_Viewport.Height = static_cast<FLOAT>(g_BackBufferDesc.Height);
    g_Viewport.MinDepth = 0.0f;
    g_Viewport.MaxDepth = 1.0f;

    g_pDeviceContext->RSSetViewports(1, &g_Viewport);

    return true;
}

/**
 * @brief バックバッファ関連のリソース解放
 */
void releaseBackBuffer()
{
    SAFE_RELEASE(g_pRenderTargetView)
        SAFE_RELEASE(g_pDepthStencilBuffer)
        SAFE_RELEASE(g_pDepthStencilView)
}

/**
 * @brief 深度テスト自体の有効/無効を設定
 */
void Direct3D_DepthStencilStateDepthIsEnable(bool isEnable)
{
    g_pDeviceContext->OMSetDepthStencilState(isEnable ? g_pDepthStencilStateDepthEnable : g_pDepthStencilStateDepthDisable, NULL);
}

/**
 * @brief 深度書き込みの有効/無効を設定
 * @detail 半透明オブジェクトや加算合成オブジェクトの描画時はfalseに設定することが推奨されます
 */
void Direct3D_SetDepthWriteEnable(bool isEnable)
{
    g_pDeviceContext->OMSetDepthStencilState(isEnable ? g_pDepthStencilStateDepthEnable : g_pDepthWriteDisable, 0);
}

/**
 * @brief ブレンドステートオブジェクトの作成
 */
HRESULT createBlendState(ID3D11BlendState** ppBlendState, D3D11_BLEND srcBlend, D3D11_BLEND destBlend, D3D11_BLEND_OP blendOp)
{
    D3D11_BLEND_DESC bd = {};
    bd.AlphaToCoverageEnable = FALSE;
    bd.IndependentBlendEnable = FALSE;
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = srcBlend;
    bd.RenderTarget[0].DestBlend = destBlend;
    bd.RenderTarget[0].BlendOp = blendOp;

    // アルファ成分の計算式設定
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    return g_pDevice->CreateBlendState(&bd, ppBlendState);
}

/**
 * @brief ラスタライザーステートの作成（通常用・影生成用）
 */
HRESULT createRasterizerState()
{
    HRESULT hr;
    D3D11_RASTERIZER_DESC rd = {};

    // 1. デフォルト設定
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_BACK;
    rd.FrontCounterClockwise = FALSE;
    rd.DepthBias = 0;
    rd.DepthBiasClamp = 0.0f;
    rd.SlopeScaledDepthBias = 0.0f;
    rd.DepthClipEnable = TRUE;
    rd.ScissorEnable = FALSE;
    rd.MultisampleEnable = FALSE;
    rd.AntialiasedLineEnable = FALSE;

    hr = g_pDevice->CreateRasterizerState(&rd, &g_pRSDefault);
    if (FAILED(hr)) return hr;

    // 2. シャドウマップ生成用設定（Depth Bias設定）
    // シャドウアクネを防ぐために深度値にオフセットを加える
    rd.CullMode = D3D11_CULL_BACK;
    rd.DepthBias = 1000;            // 固定バイアス
    rd.SlopeScaledDepthBias = 2.0f; // 傾きバイアス
    rd.DepthBiasClamp = 0.0f;

    hr = g_pDevice->CreateRasterizerState(&rd, &g_pRSShadow);

    return hr;
}