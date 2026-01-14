/****************************************
 * @file hp_gauge.h
 * @brief HPゲージ管理
 * @author Natsume Shidara
 * @date 2025/08/15
 * @update 2026/01/12 - 立体機動アクションゲーム用に改造
 ****************************************/

#ifndef HPGAUGE_H
#define HPGAUGE_H

 /**
  * @brief HPゲージ初期化
  */
void HpGauge_Initialize();

/**
 * @brief HPゲージ終了処理
 */
void HpGauge_Finalize();

/**
 * @brief HPゲージ更新
 * @param elapsed_time デルタタイム
 */
void HpGauge_Update(double elapsed_time);

/**
 * @brief HPゲージ描画
 */
void HpGauge_Draw();

#endif // HPGAUGE_H