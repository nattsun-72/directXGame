/****************************************
 * @file pad_logger.h
 * @brief ゲームパッド入力管理
 * @update 2026/01/13 - 右スティック対応追加
 ****************************************/

#ifndef PAD_LOGGER_H
#define PAD_LOGGER_H

#include <windows.h>
#include <Xinput.h>
#include <DirectXMath.h>

 /**
  * @brief パッドロガー初期化
  */
void PadLogger_Initialize();

/**
 * @brief パッドロガー更新（毎フレーム呼び出し）
 */
void PadLogger_Update();

/**
 * @brief ボタンが押されているか
 * @param userIndex コントローラーインデックス（0-3）
 * @param buttons XINPUT_GAMEPAD_* のボタンフラグ
 */
bool PadLogger_IsPressed(DWORD userIndex, WORD buttons);

/**
 * @brief ボタンが押された瞬間か
 */
bool PadLogger_IsTrigger(DWORD userIndex, WORD buttons);

/**
 * @brief ボタンが離された瞬間か
 */
bool PadLogger_IsRelease(DWORD userIndex, WORD buttons);

/**
 * @brief 左スティックの入力値を取得
 * @return X: 左右（-1.0 〜 1.0）、Y: 上下（-1.0 〜 1.0）
 */
DirectX::XMFLOAT2 PadLogger_GetLeftThumbStick(DWORD userIndex);

/**
 * @brief 右スティックの入力値を取得
 * @return X: 左右（-1.0 〜 1.0）、Y: 上下（-1.0 〜 1.0）
 */
DirectX::XMFLOAT2 PadLogger_GetRightThumbStick(DWORD userIndex);

/**
 * @brief 左トリガーの入力値を取得
 * @return 0.0 〜 1.0
 */
float PadLogger_GetLeftTrigger(DWORD userIndex);

/**
 * @brief 右トリガーの入力値を取得
 * @return 0.0 〜 1.0
 */
float PadLogger_GetRightTrigger(DWORD userIndex);

// 旧名称（互換性のため）
inline float PadLogger_GetLeftTriger(DWORD userIndex) { return PadLogger_GetLeftTrigger(userIndex); }
inline float PadLogger_GetRightTriger(DWORD userIndex) { return PadLogger_GetRightTrigger(userIndex); }

/**
 * @brief バイブレーション制御
 * @param userIndex コントローラーインデックス
 * @param enable 有効/無効
 * @param vibrationPower X: 左モーター（低周波）、Y: 右モーター（高周波）、各0.0〜1.0
 */
void PadLogger_VibrationEnable(DWORD userIndex, bool enable, DirectX::XMFLOAT2 vibrationPower = { 0.0f, 0.0f });

// 旧名称（互換性のため）
inline void PadLogger_VibrationEnalbe(DWORD userIndex, bool enable, DirectX::XMFLOAT2 vibretionPower = { 0.0f, 0.0f })
{
    PadLogger_VibrationEnable(userIndex, enable, vibretionPower);
}

#endif // PAD_LOGGER_H