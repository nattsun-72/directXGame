/****************************************
 * @file combo.h
 * @brief コンボ管理システム
 * @author Natsume Shidara
 * @date 2026/01/12
 ****************************************/

#ifndef COMBO_H
#define COMBO_H

 /**
  * @brief コンボ初期化
  */
void Combo_Initialize();

/**
 * @brief コンボ終了処理
 */
void Combo_Finalize();

/**
 * @brief コンボ更新
 * @param elapsed_time デルタタイム
 */
void Combo_Update(double elapsed_time);

/**
 * @brief コンボ描画
 */
void Combo_Draw();

/**
 * @brief コンボ追加（敵切断時に呼ぶ）
 * @param count 追加コンボ数（デフォルト1）
 */
void Combo_Add(int count = 1);

/**
 * @brief 現在のコンボ数を取得
 * @return コンボ数
 */
int Combo_GetCount();

/**
 * @brief コンボリセット
 */
void Combo_Reset();

/**
 * @brief コンボボーナススコアを取得
 * @return コンボに応じたボーナススコア
 */
int Combo_GetBonusScore();

#endif // COMBO_H