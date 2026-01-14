/****************************************
 * @file score.h
 * @brief スコア管理
 * @author Natsume Shidara
 * @date 2025/07/09
 * @update 2026/01/12 - 立体機動アクションゲーム用に改造
 ****************************************/

#ifndef SCORE_H
#define SCORE_H

 /**
  * @brief スコア初期化
  * @param x 表示X座標
  * @param y 表示Y座標
  * @param digit 表示桁数
  */
void Score_Initialize(float x, float y, int digit);

/**
 * @brief スコア終了処理
 */
void Score_Finalize();

/**
 * @brief スコア更新（カウントアップアニメーション）
 * @param elapsed_time デルタタイム
 */
void Score_Update(double elapsed_time);

/**
 * @brief スコア描画（初期化時の位置）
 */
void Score_Draw();

/**
 * @brief スコア描画（位置・サイズ指定）
 * @param x 表示X座標
 * @param y 表示Y座標
 * @param sizeX X方向スケール
 * @param sizeY Y方向スケール
 */
void Score_Draw(float x, float y, float sizeX = 1.0f, float sizeY = 1.0f);

/**
 * @brief 現在のスコアを取得
 * @return スコア値
 */
unsigned int Score_GetScore();

/**
 * @brief スコアを加算
 * @param value 加算値
 */
void Score_AddScore(int value);

/**
 * @brief スコアをリセット
 */
void Score_ResetScore();

//--------------------------------------
// リザルト画面用
//--------------------------------------

/**
 * @brief 最終スコアを保存（ゲーム終了時に呼ぶ）
 */
void Score_SaveFinal();

/**
 * @brief 最終スコアを取得
 * @return 保存された最終スコア
 */
unsigned int Score_GetFinalScore();

/**
 * @brief リザルト画面用初期化（スコアをリセットしない）
 * @param x 表示X座標
 * @param y 表示Y座標
 * @param digit 表示桁数
 */
void Score_InitializeForResult(float x, float y, int digit);

#endif // SCORE_H