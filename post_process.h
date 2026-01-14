/****************************************
 * @file post_process.h
 * @brief ポストプロセスエフェクト管理
 * @detail ラジアルブラー、モーションブラー等の画面効果
 * @author
 * @date 2026/01/10
 ****************************************/

#ifndef POST_PROCESS_H
#define POST_PROCESS_H

#include <DirectXMath.h>

 //======================================
 // エフェクト種別
 //======================================
enum class PostProcessEffect
{
    None,
    RadialBlur,     // 放射状ブラー（ダッシュ用）
    DirectionalBlur // 方向性ブラー（ステップ用）
};

//======================================
// 初期化・終了
//======================================
void PostProcess_Initialize();
void PostProcess_Finalize();

//======================================
// 描画フロー
//======================================
// シーン描画前に呼び出し（オフスクリーンへのレンダリング開始）
void PostProcess_BeginScene();

// シーン描画後に呼び出し（エフェクト適用＆バックバッファ描画）
void PostProcess_EndScene();

//======================================
// エフェクト制御
//======================================

/**
 * @brief ラジアルブラーを開始
 * @param intensity ブラー強度（0.0〜1.0、推奨: 0.3〜0.5）
 * @param duration 持続時間（秒）
 * @param fadeOut フェードアウトするか
 */
void PostProcess_StartRadialBlur(float intensity, float duration, bool fadeOut = true);

/**
 * @brief 方向性ブラーを開始（ステップ回避用）
 * @param direction ブラー方向（正規化ベクトル）
 * @param intensity ブラー強度
 * @param duration 持続時間
 */
void PostProcess_StartDirectionalBlur(const DirectX::XMFLOAT2& direction, float intensity, float duration);

/**
 * @brief 現在のエフェクトを即座に停止
 */
void PostProcess_Stop();

/**
 * @brief 更新（フェード処理等）
 * @param dt デルタタイム
 */
void PostProcess_Update(float dt);

//======================================
// 状態取得
//======================================
bool PostProcess_IsActive();
PostProcessEffect PostProcess_GetCurrentEffect();
float PostProcess_GetCurrentIntensity();

#endif // POST_PROCESS_H