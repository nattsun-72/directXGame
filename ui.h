/****************************************
 * @file ui.h
 * @brief ゲームUI管理
 * @detail HP表示などのUI要素を管理
 * @author Natsume Shidara
 * @date 2026/01/12
 ****************************************/

#ifndef UI_H
#define UI_H

 //======================================
 // UI管理関数
 //======================================

 /**
  * @brief UI初期化
  * @detail テクスチャの読み込みなど
  */
void UI_Initialize();

/**
 * @brief UI終了処理
 */
void UI_Finalize();

/**
 * @brief UI更新
 * @param dt デルタタイム
 */
void UI_Update(float dt);

/**
 * @brief UI描画
 * @detail 2D描画のため、3D描画の後に呼び出す
 */
void UI_Draw();

#endif // UI_H