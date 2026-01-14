/****************************************
 * @file pad_logger.cpp
 * @brief ゲームパッド入力管理の実装
 * @update 2026/01/13 - 右スティック対応追加
 ****************************************/

#pragma comment(lib, "xinput.lib")

#include "pad_logger.h"
#include <algorithm>

using namespace DirectX;

//======================================
// 内部変数
//======================================
static WORD g_ButtonPrev[4];
static WORD g_ButtonTrigger[4];
static WORD g_ButtonRelease[4];
static XINPUT_STATE g_State[4];

//======================================
// 初期化
//======================================
void PadLogger_Initialize()
{
    for (int i = 0; i < 4; i++)
    {
        g_ButtonPrev[i] = 0;
        g_ButtonTrigger[i] = 0;
        g_ButtonRelease[i] = 0;
        ZeroMemory(&g_State[i], sizeof(XINPUT_STATE));
    }
}

//======================================
// 更新
//======================================
void PadLogger_Update()
{
    for (int i = 0; i < 4; i++)
    {
        XInputGetState(i, &g_State[i]);

        g_ButtonTrigger[i] = (g_ButtonPrev[i] ^ g_State[i].Gamepad.wButtons) & g_State[i].Gamepad.wButtons;
        g_ButtonRelease[i] = (g_ButtonPrev[i] ^ g_State[i].Gamepad.wButtons) & g_ButtonPrev[i];

        g_ButtonPrev[i] = g_State[i].Gamepad.wButtons;
    }
}

//======================================
// ボタン入力
//======================================
bool PadLogger_IsPressed(DWORD userIndex, WORD buttons)
{
    if (userIndex >= 4) return false;
    return (g_State[userIndex].Gamepad.wButtons & buttons) != 0;
}

bool PadLogger_IsTrigger(DWORD userIndex, WORD buttons)
{
    if (userIndex >= 4) return false;
    return (g_ButtonTrigger[userIndex] & buttons) != 0;
}

bool PadLogger_IsRelease(DWORD userIndex, WORD buttons)
{
    if (userIndex >= 4) return false;
    return (g_ButtonRelease[userIndex] & buttons) != 0;
}

//======================================
// 左スティック
//======================================
XMFLOAT2 PadLogger_GetLeftThumbStick(DWORD userIndex)
{
    if (userIndex >= 4) return { 0.0f, 0.0f };

    XMFLOAT2 value = { 0.0f, 0.0f };
    SHORT lx = g_State[userIndex].Gamepad.sThumbLX;
    SHORT ly = g_State[userIndex].Gamepad.sThumbLY;

    // X軸
    if (lx < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        value.x = static_cast<float>(lx + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) /
            static_cast<float>(32768 - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    }
    else if (lx > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        value.x = static_cast<float>(lx - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) /
            static_cast<float>(32767 - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    }

    // Y軸
    if (ly < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        value.y = static_cast<float>(ly + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) /
            static_cast<float>(32768 - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    }
    else if (ly > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        value.y = static_cast<float>(ly - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) /
            static_cast<float>(32767 - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    }

    // 正規化（大きさが1を超えないように）
    float lengthSq = value.x * value.x + value.y * value.y;
    if (lengthSq > 1.0f)
    {
        float length = sqrtf(lengthSq);
        value.x /= length;
        value.y /= length;
    }

    return value;
}

//======================================
// 右スティック
//======================================
XMFLOAT2 PadLogger_GetRightThumbStick(DWORD userIndex)
{
    if (userIndex >= 4) return { 0.0f, 0.0f };

    XMFLOAT2 value = { 0.0f, 0.0f };
    SHORT rx = g_State[userIndex].Gamepad.sThumbRX;
    SHORT ry = g_State[userIndex].Gamepad.sThumbRY;

    // X軸
    if (rx < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
    {
        value.x = static_cast<float>(rx + XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) /
            static_cast<float>(32768 - XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    }
    else if (rx > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
    {
        value.x = static_cast<float>(rx - XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) /
            static_cast<float>(32767 - XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    }

    // Y軸
    if (ry < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
    {
        value.y = static_cast<float>(ry + XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) /
            static_cast<float>(32768 - XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    }
    else if (ry > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
    {
        value.y = static_cast<float>(ry - XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) /
            static_cast<float>(32767 - XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    }

    // 正規化（大きさが1を超えないように）
    float lengthSq = value.x * value.x + value.y * value.y;
    if (lengthSq > 1.0f)
    {
        float length = sqrtf(lengthSq);
        value.x /= length;
        value.y /= length;
    }

    return value;
}

//======================================
// トリガー
//======================================
float PadLogger_GetLeftTrigger(DWORD userIndex)
{
    if (userIndex >= 4) return 0.0f;

    BYTE trigger = g_State[userIndex].Gamepad.bLeftTrigger;
    if (trigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        return 0.0f;

    return static_cast<float>(trigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) /
        static_cast<float>(255 - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

float PadLogger_GetRightTrigger(DWORD userIndex)
{
    if (userIndex >= 4) return 0.0f;

    BYTE trigger = g_State[userIndex].Gamepad.bRightTrigger;
    if (trigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        return 0.0f;

    return static_cast<float>(trigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) /
        static_cast<float>(255 - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

//======================================
// バイブレーション
//======================================
void PadLogger_VibrationEnable(DWORD userIndex, bool enable, XMFLOAT2 vibrationPower)
{
    if (userIndex >= 4) return;

    if (enable)
    {
        vibrationPower.x = std::clamp(vibrationPower.x, 0.0f, 1.0f);
        vibrationPower.y = std::clamp(vibrationPower.y, 0.0f, 1.0f);

        XINPUT_VIBRATION xv;
        xv.wLeftMotorSpeed = static_cast<WORD>(vibrationPower.x * 65535);
        xv.wRightMotorSpeed = static_cast<WORD>(vibrationPower.y * 65535);

        XInputSetState(userIndex, &xv);
    }
    else
    {
        XINPUT_VIBRATION xv = { 0, 0 };
        XInputSetState(userIndex, &xv);
    }
}