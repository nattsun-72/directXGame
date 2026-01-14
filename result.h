/****************************************
 * @file result.h
 * @brief リザルト画面
 * @author Natsume Shidara
 * @date 2025/08/19
 * @update 2026/01/12 - 立体機動アクションゲーム用に改造
 ****************************************/

#ifndef RESULT_H
#define RESULT_H

 /**
  * @brief リザルト画面初期化
  */
void Result_Initialize();

/**
 * @brief リザルト画面終了処理
 */
void Result_Finalize();

/**
 * @brief リザルト画面更新
 * @param elapsed_time デルタタイム
 */
void Result_Update(double elapsed_time);

/**
 * @brief リザルト画面描画
 */
void Result_Draw();

/**
 * @brief リザルト画面のシャドウ描画（空実装）
 */
void Result_DrawShadow();

#endif // RESULT_H