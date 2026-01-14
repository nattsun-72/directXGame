/****************************************
 * @file game_settings.h
 * @brief ゲーム設定の管理
 * @author Natsume Shidara
 * @date 2026/01/13
 *
 * 各種設定値を保持し、ゲーム全体から参照可能にする
 ****************************************/

#ifndef GAME_SETTINGS_H
#define GAME_SETTINGS_H

 //======================================
 // 設定値の範囲
 //======================================
namespace SettingsRange
{
    // 視点感度
    constexpr float SENSITIVITY_MIN = 0.1f;
    constexpr float SENSITIVITY_MAX = 3.0f;
    constexpr float SENSITIVITY_DEFAULT = 1.0f;

    // 音量
    constexpr float VOLUME_MIN = 0.0f;
    constexpr float VOLUME_MAX = 1.0f;
    constexpr float BGM_VOLUME_DEFAULT = 0.3f;
    constexpr float SE_VOLUME_DEFAULT = 0.3f;
}

//======================================
// 初期化・終了
//======================================
void GameSettings_Initialize();
void GameSettings_Finalize();

//======================================
// 視点感度
//======================================
float GameSettings_GetSensitivity();
void GameSettings_SetSensitivity(float value);

//======================================
// BGM音量
//======================================
float GameSettings_GetBGMVolume();
void GameSettings_SetBGMVolume(float value);

//======================================
// SE音量
//======================================
float GameSettings_GetSEVolume();
void GameSettings_SetSEVolume(float value);

//======================================
// 設定のリセット
//======================================
void GameSettings_ResetToDefault();

#endif // GAME_SETTINGS_H