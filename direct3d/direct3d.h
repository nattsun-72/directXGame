/****************************************
 * @file direct3d.h
 * @brief Direct3Dの初期化およびデバイス管理
 * @author Natsume Shidara
 * @date 2025/06/06
 * @update 2025/12/10
 ****************************************/

#ifndef DIRECT3D_H
#define DIRECT3D_H

#include <Windows.h>
#include <d3d11.h>
#include <DirectXMath.h>

 //--------------------------------------
 // マクロ定義
 //--------------------------------------
 // セーフリリースマクロ
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p){ (p)->Release(); (p)=NULL; } }
#endif

//======================================
// 関数プロトタイプ宣言
//======================================

//--------------------------------------
// 基本初期化・終了処理
//--------------------------------------
/**
 * @brief Direct3Dの初期化
 * @param hWnd ウィンドウハンドル
 * @return 成功時 true
 */
bool Direct3D_Initialize(HWND hWnd);

/**
 * @brief Direct3Dの終了処理
 */
void Direct3D_Finalize();

//--------------------------------------
// 描画処理
//--------------------------------------
/**
 * @brief バックバッファのクリア
 */
void Direct3D_Clear();

/**
 * @brief バックバッファの表示（フリップ）
 */
void Direct3D_Present();

/**
 * @brief 描画先をバックバッファ（画面）に戻す
 * @detail RenderTargetクラス等でオフスクリーン描画を行った後に使用する
 */
void Direct3D_SetBackBufferRenderTarget();

//--------------------------------------
// 情報取得
//--------------------------------------
unsigned int Direct3D_GetBackBufferWidth();
unsigned int Direct3D_GetBackBufferHeight();
ID3D11Device* Direct3D_GetDevice();
ID3D11DeviceContext* Direct3D_GetContext();

//--------------------------------------
// 深度ステンシル制御
//--------------------------------------
/**
 * @brief 深度テストの有効/無効切り替え
 * @param isEnable true:有効(通常) / false:無効(2D描画等)
 */
void Direct3D_DepthStencilStateDepthIsEnable(bool isEnable = true);

/**
 * @brief 深度書き込みの有効/無効切り替え
 * @param isEnable true:書き込み有効 / false:書き込み無効(半透明描画用)
 */
void Direct3D_SetDepthWriteEnable(bool isEnable = true);

//--------------------------------------
// ブレンドモード
//--------------------------------------
enum class BlendMode
{
    None,      // ブレンドなし（不透明）
    Alpha,     // 通常（アルファブレンド）
    Add,       // 加算合成
    Multiply   // 乗算合成
};

/**
 * @brief ブレンドステートの設定
 * @param mode 設定したいブレンドモード
 */
void Direct3D_SetBlendState(BlendMode mode);

//--------------------------------------
// 座標変換ヘルパー
//--------------------------------------
/**
 * @brief ビューポート行列の取得
 */
DirectX::XMMATRIX Direct3D_MatrixViewPort();

/**
 * @brief スクリーン座標からワールド座標への変換（レイキャスト用など）
 */
DirectX::XMFLOAT3 Direct3D_ScreenToWorld(int x, int y, float depth,
                                         const DirectX::XMFLOAT4X4& view,
                                         const DirectX::XMFLOAT4X4& projection);

/**
 * @brief ワールド座標からスクリーン座標への変換（UI追従など）
 */
DirectX::XMFLOAT2 Direct3D_WorldToScreen(const DirectX::XMFLOAT3& position,
                                         const DirectX::XMFLOAT4X4& view,
                                         const DirectX::XMFLOAT4X4& projection);

// ===== ラスタライザーステート =====
void Direct3D_SetRasterizerState_Shadow(); // 影生成用（バイアスあり）
void Direct3D_SetRasterizerState_Default(); // 通常描画用
#endif // DIRECT3D_H