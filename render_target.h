/****************************************
 * @file render_target.h
 * @brief オフスクリーンレンダリング用ターゲット管理
 * @author Natsume Shidara
 * @date 2025/12/10
 * @update 2025/12/10
 ****************************************/

#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include <d3d11.h>
#include <DirectXMath.h>

 //--------------------------------------
 // 列挙体定義
 //--------------------------------------
 /****************************************
  * @enum RenderTargetType
  * @brief 作成するレンダーターゲットの種類
  ****************************************/
enum class RenderTargetType
{
    Color,  // 通常の描画（ミニマップ、ポストエフェクト用）
    Shadow  // 深度描画（シャドウマップ用）
};

//--------------------------------------
// クラス宣言
//--------------------------------------
/****************************************
 * @class RenderTarget
 * @brief テクスチャへの描画機能を提供するクラス
 *
 * ミニマップ作成（Colorモード）や、
 * シャドウマップ作成（Shadowモード）に使用する。
 * 描画先（RTV/DSV）と、参照用（SRV）を管理する。
 ****************************************/
class RenderTarget {
private:
    //--------------------------------------
    // メンバ変数群
    //--------------------------------------
    ID3D11Texture2D* m_pTexture;      // メインテクスチャ（色 or 深度）
    ID3D11RenderTargetView* m_pRTV;          // レンダーターゲットビュー（Colorモード用）
    ID3D11DepthStencilView* m_pDSV;          // 深度ステンシルビュー
    ID3D11ShaderResourceView* m_pSRV;          // シェーダーリソースビュー（テクスチャとして使用）
    ID3D11Texture2D* m_pDepthTexture; // 深度用テクスチャ（Colorモード時の裏方用）

    D3D11_VIEWPORT            m_Viewport;      // ビューポート
    RenderTargetType          m_Type;          // 作成タイプ
    float                     m_ClearColor[4]; // Colorモード時のクリア色

public:
    //======================================
    // コンストラクタ／デストラクタ
    //======================================
    RenderTarget();
    ~RenderTarget();

    //======================================
    // 初期化・解放
    //======================================
    /**
     * @brief レンダーターゲット生成
     * @param width 幅
     * @param height 高さ
     * @param type 作成タイプ（Color:ミニマップ等, Shadow:影）
     * @return 成功true
     */
    bool Initialize(unsigned int width, unsigned int height, RenderTargetType type);

    /**
     * @brief 解放処理
     */
    void Finalize();

    //======================================
    // 描画制御
    //======================================
    /**
     * @brief 描画先をこのターゲットに切り替える
     * @detail 画面クリアも同時に行われる
     */
    void Activate();

    /**
     * @brief 背景クリア色の設定（Colorモード用）
     */
    void SetClearColor(float r, float g, float b, float a);

    //======================================
    // アクセサ
    //======================================
    /**
     * @brief 作成したテクスチャのSRV取得
     * @return シェーダーに渡すリソースビュー
     */
    ID3D11ShaderResourceView* GetSRV() const;
};

#endif // RENDER_TARGET_H