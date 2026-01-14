/****************************************
 * @file shader3d.h
 * @brief シェーダー（3D描画・ShadowMap対応）
 * @author Natsume Shidara
 * @date 2025/09/10
 * @update 2025/12/10
 ****************************************/

#ifndef SHADER3D_H
#define SHADER3D_H

#include <d3d11.h>
#include <DirectXMath.h>

 //=============================================================================
 // 初期化・終了処理
 //=============================================================================
 /**
  * @brief シェーダーリソースの初期化
  * @detail 頂点・ピクセルシェーダーの読み込み、定数バッファの作成を行う
  * @param pDevice Direct3Dデバイス
  * @param pContext Direct3Dデバイスコンテキスト
  * @return 成功時はtrue、失敗時はfalse
  */
bool Shader3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

/**
 * @brief シェーダーリソースの解放
 */
void Shader3D_Finalize();

//=============================================================================
// パラメータ設定関数群
//=============================================================================
/**
 * @brief ワールド行列の設定 (Vertex Shader b0)
 * @param matrix ワールド行列
 */
void Shader3D_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

/**
 * @brief ライトビュープロジェクション行列の設定 (Vertex Shader b3)
 * @detail シャドウマップ生成時のライト視点行列を設定する
 * @param matrix ライトビュープロジェクション行列
 */
void Shader3D_SetLightViewProjection(const DirectX::XMFLOAT4X4& matrix);

/**
 * @brief シャドウマップテクスチャの設定 (Pixel Shader Slot 1)
 * @param pShadowSRV 深度バッファから作成されたSRV
 */
void Shader3D_SetShadowMap(ID3D11ShaderResourceView* pShadowSRV);

/**
 * @brief マテリアルカラーの設定 (Pixel Shader b0)
 * @param color RGBAカラー
 */
void Shader3D_SetColor(const DirectX::XMFLOAT4& color);

//=============================================================================
// 描画開始
//=============================================================================
/**
 * @brief シェーダーの有効化
 * @detail シェーダー、レイアウト、定数バッファ、サンプラをコンテキストに設定する
 */
void Shader3D_Begin();

#endif // SHADER3D_H