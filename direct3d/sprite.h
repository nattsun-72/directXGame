/**
 * @file sprite.h
 * @brief スプライト表示
 * @author Natsume Shidara
 * @date 2025/06/12
 * @update 2025/12/10
 */

#ifndef SPRITE_H
#define SPRITE_H

#include <d3d11.h>
#include <DirectXMath.h>

 // 初期化・終了
void Sprite_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Sprite_Finalize(void);

// 描画開始
void Sprite_Begin();

//-------------------------------------------------------------
// 既存の描画関数（テクスチャID指定）
//-------------------------------------------------------------
// 通常描画
void Sprite_Draw(int texid, float display_x, float display_y, float angle = 0.0f, const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1, 1, 1, 1));

// サイズ指定描画
void Sprite_Draw(int texid, float display_x, float display_y, float display_w, float display_h, float angle = 0.0f, const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1, 1, 1, 1));

// 切り抜き描画
void Sprite_Draw(int texid, float display_x, float display_y, float uvcut_x, float uvcut_y, float uvcut_w, float uvcut_h, float angle = 0.0f, const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1, 1, 1, 1));

// フルスペック描画
void Sprite_Draw(int texid, float display_x, float display_y, float uvcut_x, float uvcut_y, float uvcut_w, float uvcut_h, float display_w, float display_h, float angle = 0.0f, const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1, 1, 1, 1));

//-------------------------------------------------------------
//SRV直接描画関数（オフスクリーンレンダリング結果などの描画用）
//-------------------------------------------------------------
/**
 * @brief シェーダーリソースビューを直接指定して描画
 * @param pSRV 描画したいテクスチャのリソースビュー
 * @param display_x 画面X座標
 * @param display_y 画面Y座標
 * @param display_w 描画幅
 * @param display_h 描画高さ
 * @param angle 回転角度（ラジアン）
 * @param color 頂点カラー
 */
void Sprite_Draw(ID3D11ShaderResourceView* pSRV, float display_x, float display_y, float display_w, float display_h, float angle = 0.0f, const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1, 1, 1, 1));

#endif // SPRITE_H