/****************************************
 * @file game_timer.h
 * @brief ゲームタイマー管理（残り時間表示）
 * @author Natsume Shidara
 * @date 2026/01/12
 ****************************************/

#ifndef GAME_TIMER_H
#define GAME_TIMER_H

 /**
  * @brief ゲームタイマー初期化
  * @param totalTime 制限時間（秒）
  */
void GameTimer_Initialize(float totalTime);

/**
 * @brief ゲームタイマー終了処理
 */
void GameTimer_Finalize();

/**
 * @brief ゲームタイマー更新
 * @param elapsed_time デルタタイム
 */
void GameTimer_Update(double elapsed_time);

/**
 * @brief ゲームタイマー描画
 */
void GameTimer_Draw();

/**
 * @brief 残り時間を取得
 * @return 残り時間（秒）
 */
float GameTimer_GetRemaining();

/**
 * @brief タイムアップかどうか
 * @return タイムアップならtrue
 */
bool GameTimer_IsTimeUp();

/**
 * @brief タイマーを一時停止
 */
void GameTimer_Pause();

/**
 * @brief タイマーを再開
 */
void GameTimer_Resume();

/**
 * @brief タイマーをリセット
 * @param totalTime 制限時間（秒）
 */
void GameTimer_Reset(float totalTime);

#endif // GAME_TIMER_H