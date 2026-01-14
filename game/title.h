/****************************************
 * @file title.h
 * @brief タイトル画面「斬空 -ZANKUU-」
 * @author Natsume Shidara
 * @date 2026/01/12
 ****************************************/

#ifndef TITLE_H
#define TITLE_H

 /**
  * @brief タイトル画面初期化
  */
void Title_Initialize();

/**
 * @brief タイトル画面終了処理
 */
void Title_Finalize();

/**
 * @brief タイトル画面更新
 * @param elapsed_time デルタタイム
 */
void Title_Update(double elapsed_time);

/**
 * @brief タイトル画面描画
 */
void Title_Draw();

/**
 * @brief タイトル画面シャドウ描画（空実装）
 */
void Title_DrawShadow();

#endif // TITLE_H
