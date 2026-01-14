/****************************************
 * @file game_settings.cpp
 * @brief ゲーム設定の管理
 * @author Natsume Shidara
 * @date 2026/01/13
 ****************************************/

#include "game_settings.h"
#include "sound_manager.h"
#include <algorithm>

 //======================================
 // 内部変数
 //======================================
static float g_Sensitivity = SettingsRange::SENSITIVITY_DEFAULT;
static float g_BGMVolume = SettingsRange::BGM_VOLUME_DEFAULT;
static float g_SEVolume = SettingsRange::SE_VOLUME_DEFAULT;

//======================================
// ヘルパー関数
//======================================
static float Clamp(float value, float min, float max)
{
    return std::max(min, std::min(max, value));
}

//======================================
// 初期化・終了
//======================================

void GameSettings_Initialize()
{
    // デフォルト値で初期化
    g_Sensitivity = SettingsRange::SENSITIVITY_DEFAULT;
    g_BGMVolume = SettingsRange::BGM_VOLUME_DEFAULT;
    g_SEVolume = SettingsRange::SE_VOLUME_DEFAULT;

    // サウンドマネージャーに初期値を適用
    SoundManager_SetBGMVolume(g_BGMVolume);
    SoundManager_SetSEVolume(g_SEVolume);
}

void GameSettings_Finalize()
{
    // 現時点では特に何もしない
    // 将来的にはファイルへの保存などを行う
}

//======================================
// 視点感度
//======================================

float GameSettings_GetSensitivity()
{
    return g_Sensitivity;
}

void GameSettings_SetSensitivity(float value)
{
    g_Sensitivity = Clamp(value, SettingsRange::SENSITIVITY_MIN, SettingsRange::SENSITIVITY_MAX);
}

//======================================
// BGM音量
//======================================

float GameSettings_GetBGMVolume()
{
    return g_BGMVolume;
}

void GameSettings_SetBGMVolume(float value)
{
    g_BGMVolume = Clamp(value, SettingsRange::VOLUME_MIN, SettingsRange::VOLUME_MAX);
    SoundManager_SetBGMVolume(g_BGMVolume);
}

//======================================
// SE音量
//======================================

float GameSettings_GetSEVolume()
{
    return g_SEVolume;
}

void GameSettings_SetSEVolume(float value)
{
    g_SEVolume = Clamp(value, SettingsRange::VOLUME_MIN, SettingsRange::VOLUME_MAX);
    SoundManager_SetSEVolume(g_SEVolume);
}

//======================================
// 設定のリセット
//======================================

void GameSettings_ResetToDefault()
{
    GameSettings_SetSensitivity(SettingsRange::SENSITIVITY_DEFAULT);
    GameSettings_SetBGMVolume(SettingsRange::BGM_VOLUME_DEFAULT);
    GameSettings_SetSEVolume(SettingsRange::SE_VOLUME_DEFAULT);
}