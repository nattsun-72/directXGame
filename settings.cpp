/****************************************
 * @file settings.cpp
 * @brief 設定画面の実装
 * @author Natsume Shidara
 * @date 2026/01/13
 *
 * 設定項目:
 * - 視点感度
 * - BGM音量
 * - SE音量
 ****************************************/

#include "settings.h"
#include "scene.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
#include "key_logger.h"
#include "pad_logger.h"
#include "fade.h"
#include "game_settings.h"
#include "sound_manager.h"
#include "debug_text.h"

#include <DirectXMath.h>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>

using namespace DirectX;

//======================================
// 定数
//======================================
namespace
{
    // テクスチャパス
    constexpr const wchar_t* TEX_BG = L"assets/ui/settings_bg.png";
    constexpr const wchar_t* TEX_WHITE = L"assets/white.png";
    constexpr const wchar_t* TEX_TITLE = L"assets/ui/settings_title.png";

    // フォントテクスチャパス
    constexpr const wchar_t* FONT_TEXTURE_PATH = L"assets/consolab_ascii_512.png";

    // メニュー項目
    enum MenuItem
    {
        MENU_SENSITIVITY,
        MENU_BGM_VOLUME,
        MENU_SE_VOLUME,
        MENU_RESET,
        MENU_BACK,
        MENU_COUNT
    };

    // メニュー項目名
    const char* MENU_LABELS[MENU_COUNT] = {
        "Mouse Sensitivity",
        "BGM Volume",
        "SE Volume",
        "Reset to Default",
        "Back to Title"
    };

    // レイアウト
    constexpr float TITLE_Y = 50.0f;
    constexpr float MENU_START_Y = 180.0f;
    constexpr float MENU_SPACING = 70.0f;
    constexpr float LABEL_X = 100.0f;
    constexpr float SLIDER_X = 500.0f;
    constexpr float SLIDER_WIDTH = 300.0f;
    constexpr float SLIDER_HEIGHT = 20.0f;
    constexpr float KNOB_WIDTH = 16.0f;
    constexpr float KNOB_HEIGHT = 30.0f;
    constexpr float BUTTON_WIDTH = 180.0f;
    constexpr float BUTTON_HEIGHT = 40.0f;
    constexpr float CURSOR_HEIGHT = 50.0f;

    // 調整速度
    constexpr float ADJUST_SPEED = 0.015f;

    // アニメーション
    constexpr float BLINK_SPEED = 5.0f;
    constexpr float CURSOR_ANIM_SPEED = 3.0f;
}

//======================================
// 内部変数
//======================================
static int g_TexBg = -1;
static int g_TexWhite = -1;
static int g_TexTitle = -1;

static float g_ScreenWidth = 0.0f;
static float g_ScreenHeight = 0.0f;

static int g_SelectedItem = 0;
static float g_AnimTimer = 0.0f;
static bool g_IsTransitioning = false;

// 入力リピート用
static float g_InputRepeatTimer = 0.0f;
static constexpr float INPUT_REPEAT_DELAY = 0.25f;
static constexpr float INPUT_REPEAT_INTERVAL = 0.03f;
static bool g_FirstInput = true;

// デバッグテキスト
static hal::DebugText* g_pDebugText = nullptr;

//======================================
// 内部関数プロトタイプ
//======================================
static void DrawSlider(float x, float y, float value, float minVal, float maxVal, bool selected);
static void DrawButton(float x, float y, const char* text, bool selected);
static float GetCurrentValue(int item);
static void SetCurrentValue(int item, float value);
static void AdjustValue(int item, float delta);

//======================================
// 初期化
//======================================
void Settings_Initialize()
{
    g_ScreenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    g_ScreenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());

    // テクスチャ読み込み
    g_TexBg = Texture_Load(TEX_BG);
    g_TexWhite = Texture_Load(TEX_WHITE);
    g_TexTitle = Texture_Load(TEX_TITLE);

    // デバッグテキスト初期化
    g_pDebugText = new hal::DebugText(
        Direct3D_GetDevice(),
        Direct3D_GetContext(),
        FONT_TEXTURE_PATH,
        static_cast<int>(g_ScreenWidth),
        static_cast<int>(g_ScreenHeight),
        0.0f, 0.0f, 0, 0, 0.0f, 24.0f
    );

    g_SelectedItem = 0;
    g_AnimTimer = 0.0f;
    g_IsTransitioning = false;
    g_InputRepeatTimer = 0.0f;
    g_FirstInput = true;

    // フェードイン
    if (Fade_GetState() == FADE_STATE::FINISHED_OUT)
    {
        Fade_Start(0.5, false);
    }
}

//======================================
// 終了処理
//======================================
void Settings_Finalize()
{
    if (g_pDebugText)
    {
        delete g_pDebugText;
        g_pDebugText = nullptr;
    }

    g_TexBg = -1;
    g_TexWhite = -1;
    g_TexTitle = -1;
}

//======================================
// スライダー描画
//======================================
static void DrawSlider(float x, float y, float value, float minVal, float maxVal, bool selected)
{
    if (g_TexWhite < 0) return;

    // 正規化された値
    float normalizedValue = (value - minVal) / (maxVal - minVal);
    normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);

    // スライダー背景
    XMFLOAT4 bgColor = { 0.2f, 0.2f, 0.25f, 0.9f };
    Sprite_Draw(g_TexWhite, x, y, 0.0f, 0.0f, 1.0f, 1.0f,
        SLIDER_WIDTH, SLIDER_HEIGHT, 0.0f, bgColor);

    // 値バー（塗りつぶし部分）
    float barWidth = SLIDER_WIDTH * normalizedValue;
    if (barWidth > 0)
    {
        float pulse = selected ? (0.8f + 0.2f * sinf(g_AnimTimer * BLINK_SPEED)) : 0.7f;
        XMFLOAT4 barColor = { 0.3f * pulse, 0.6f * pulse, 1.0f * pulse, 0.9f };
        Sprite_Draw(g_TexWhite, x, y, 0.0f, 0.0f, 1.0f, 1.0f,
            barWidth, SLIDER_HEIGHT, 0.0f, barColor);
    }

    // つまみ（ノブ）
    float knobX = x + barWidth - KNOB_WIDTH * 0.5f;
    float knobY = y - (KNOB_HEIGHT - SLIDER_HEIGHT) * 0.5f;

    XMFLOAT4 knobColor = selected ?
        XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f } :
        XMFLOAT4{ 0.85f, 0.85f, 0.9f, 1.0f };

    Sprite_Draw(g_TexWhite, knobX, knobY, 0.0f, 0.0f, 1.0f, 1.0f,
        KNOB_WIDTH, KNOB_HEIGHT, 0.0f, knobColor);
}

//======================================
// ボタン描画
//======================================
static void DrawButton(float x, float y, const char* text, bool selected)
{
    if (g_TexWhite < 0) return;

    float alpha = 1.0f;
    if (selected)
    {
        alpha = 0.85f + 0.15f * sinf(g_AnimTimer * BLINK_SPEED);
    }

    XMFLOAT4 btnColor = selected ?
        XMFLOAT4{ 0.3f, 0.5f, 0.8f, alpha } :
        XMFLOAT4{ 0.2f, 0.25f, 0.35f, 0.8f };

    Sprite_Draw(g_TexWhite, x, y, 0.0f, 0.0f, 1.0f, 1.0f,
        BUTTON_WIDTH, BUTTON_HEIGHT, 0.0f, btnColor);
}

//======================================
// 設定値取得
//======================================
static float GetCurrentValue(int item)
{
    switch (item)
    {
    case MENU_SENSITIVITY:
        return GameSettings_GetSensitivity();
    case MENU_BGM_VOLUME:
        return GameSettings_GetBGMVolume();
    case MENU_SE_VOLUME:
        return GameSettings_GetSEVolume();
    default:
        return 0.0f;
    }
}

//======================================
// 設定値設定
//======================================
static void SetCurrentValue(int item, float value)
{
    switch (item)
    {
    case MENU_SENSITIVITY:
        GameSettings_SetSensitivity(value);
        break;
    case MENU_BGM_VOLUME:
        GameSettings_SetBGMVolume(value);
        break;
    case MENU_SE_VOLUME:
        GameSettings_SetSEVolume(value);
        break;
    }
}

//======================================
// 値調整
//======================================
static void AdjustValue(int item, float delta)
{
    float current = GetCurrentValue(item);
    float newValue = current + delta;

    switch (item)
    {
    case MENU_SENSITIVITY:
        newValue = std::clamp(newValue, SettingsRange::SENSITIVITY_MIN, SettingsRange::SENSITIVITY_MAX);
        break;
    case MENU_BGM_VOLUME:
    case MENU_SE_VOLUME:
        newValue = std::clamp(newValue, SettingsRange::VOLUME_MIN, SettingsRange::VOLUME_MAX);
        break;
    }

    SetCurrentValue(item, newValue);
}

//======================================
// 更新
//======================================
void Settings_Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);

    g_AnimTimer += dt;

    if (g_IsTransitioning)
    {
        if (Fade_GetState() == FADE_STATE::FINISHED_OUT)
        {
            Scene_Change(Scene::TITLE);
        }
        return;
    }

    // 上下移動
    bool upInput = KeyLogger_IsTrigger(KK_W) || KeyLogger_IsTrigger(KK_UP) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_DPAD_UP);
    bool downInput = KeyLogger_IsTrigger(KK_S) || KeyLogger_IsTrigger(KK_DOWN) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_DPAD_DOWN);

    // パッドスティック（上下）
    XMFLOAT2 stick = PadLogger_GetLeftThumbStick(0);
    static bool stickUpPrev = false;
    static bool stickDownPrev = false;
    bool stickUp = stick.y > 0.5f;
    bool stickDown = stick.y < -0.5f;

    if (stickUp && !stickUpPrev) upInput = true;
    if (stickDown && !stickDownPrev) downInput = true;
    stickUpPrev = stickUp;
    stickDownPrev = stickDown;

    if (upInput)
    {
        g_SelectedItem--;
        if (g_SelectedItem < 0) g_SelectedItem = MENU_COUNT - 1;
        SoundManager_PlaySE(SOUND_SE_SELECT);
    }
    if (downInput)
    {
        g_SelectedItem++;
        if (g_SelectedItem >= MENU_COUNT) g_SelectedItem = 0;
        SoundManager_PlaySE(SOUND_SE_SELECT);
    }

    // 左右でスライダー調整（リピート対応）
    bool leftHeld = KeyLogger_IsPressed(KK_A) || KeyLogger_IsPressed(KK_LEFT) ||
        PadLogger_IsPressed(0, XINPUT_GAMEPAD_DPAD_LEFT) ||
        (stick.x < -0.3f);
    bool rightHeld = KeyLogger_IsPressed(KK_D) || KeyLogger_IsPressed(KK_RIGHT) ||
        PadLogger_IsPressed(0, XINPUT_GAMEPAD_DPAD_RIGHT) ||
        (stick.x > 0.3f);

    if (leftHeld || rightHeld)
    {
        if (g_SelectedItem <= MENU_SE_VOLUME)
        {
            g_InputRepeatTimer += dt;

            bool shouldAdjust = false;
            if (g_FirstInput)
            {
                shouldAdjust = true;
                g_FirstInput = false;
            }
            else if (g_InputRepeatTimer >= INPUT_REPEAT_DELAY)
            {
                shouldAdjust = true;
                g_InputRepeatTimer = INPUT_REPEAT_DELAY - INPUT_REPEAT_INTERVAL;
            }

            if (shouldAdjust)
            {
                float delta = rightHeld ? ADJUST_SPEED : -ADJUST_SPEED;

                if (g_SelectedItem == MENU_SENSITIVITY)
                {
                    delta *= (SettingsRange::SENSITIVITY_MAX - SettingsRange::SENSITIVITY_MIN);
                }

                AdjustValue(g_SelectedItem, delta);
            }
        }
    }
    else
    {
        g_InputRepeatTimer = 0.0f;
        g_FirstInput = true;
    }

    // 決定ボタン
    bool confirmInput = KeyLogger_IsTrigger(KK_ENTER) || KeyLogger_IsTrigger(KK_SPACE) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_A);

    if (confirmInput)
    {
        switch (g_SelectedItem)
        {
        case MENU_RESET:
            GameSettings_ResetToDefault();
            SoundManager_PlaySE(SOUND_SE_DECIDE);
            break;

        case MENU_BACK:
            SoundManager_PlaySE(SOUND_SE_DECIDE);
            g_IsTransitioning = true;
            Fade_Start(0.5, true);
            break;

        default:
            if (g_SelectedItem == MENU_SE_VOLUME)
            {
                SoundManager_PlaySE(SOUND_SE_DECIDE);
            }
            break;
        }
    }

    // キャンセル
    bool cancelInput = KeyLogger_IsTrigger(KK_ESCAPE) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_B);

    if (cancelInput)
    {
        SoundManager_PlaySE(SOUND_SE_CANCEL);
        g_IsTransitioning = true;
        Fade_Start(0.5, true);
    }
}

//======================================
// 描画
//======================================
void Settings_Draw()
{
    Direct3D_DepthStencilStateDepthIsEnable(false);
    Direct3D_SetBlendState(BlendMode::Alpha);

    Sprite_Begin();

    // 背景
    if (g_TexBg >= 0)
    {
        Sprite_Draw(g_TexBg, 0.0f, 0.0f, g_ScreenWidth, g_ScreenHeight);
    }
    else if (g_TexWhite >= 0)
    {
        XMFLOAT4 bgColor = { 0.08f, 0.08f, 0.12f, 1.0f };
        Sprite_Draw(g_TexWhite, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            g_ScreenWidth, g_ScreenHeight, 0.0f, bgColor);
    }

    // タイトル
    if (g_TexTitle >= 0)
    {
        float titleWidth = 400.0f;
        float titleHeight = 80.0f;
        float titleX = (g_ScreenWidth - titleWidth) * 0.5f;
        Sprite_Draw(g_TexTitle, titleX, TITLE_Y, titleWidth, titleHeight);
    }

    // 各メニュー項目を描画
    for (int i = 0; i < MENU_COUNT; i++)
    {
        float y = MENU_START_Y + i * MENU_SPACING;
        bool selected = (i == g_SelectedItem);

        // 選択カーソル
        if (selected && g_TexWhite >= 0)
        {
            float cursorAlpha = 0.3f + 0.15f * sinf(g_AnimTimer * CURSOR_ANIM_SPEED);
            XMFLOAT4 cursorColor = { 0.3f, 0.5f, 0.9f, cursorAlpha };
            Sprite_Draw(g_TexWhite, LABEL_X - 20.0f, y - 10.0f,
                0.0f, 0.0f, 1.0f, 1.0f,
                g_ScreenWidth - LABEL_X - 50.0f, CURSOR_HEIGHT, 0.0f, cursorColor);
        }

        // スライダーまたはボタン
        switch (i)
        {
        case MENU_SENSITIVITY:
            DrawSlider(SLIDER_X, y + 3.0f, GetCurrentValue(i),
                SettingsRange::SENSITIVITY_MIN, SettingsRange::SENSITIVITY_MAX, selected);
            break;

        case MENU_BGM_VOLUME:
        case MENU_SE_VOLUME:
            DrawSlider(SLIDER_X, y + 3.0f, GetCurrentValue(i),
                SettingsRange::VOLUME_MIN, SettingsRange::VOLUME_MAX, selected);
            break;

        case MENU_RESET:
        case MENU_BACK:
            DrawButton(SLIDER_X, y - 5.0f, (i == MENU_RESET) ? "RESET" : "BACK", selected);
            break;
        }
    }

    // テキスト描画（DebugText使用）
    if (g_pDebugText)
    {
        std::stringstream ss;

        // タイトル（画像がない場合）
        if (g_TexTitle < 0)
        {
            ss << "\n";
            ss << "                    SETTINGS\n";
            ss << "\n";
        }
        else
        {
            ss << "\n\n\n\n\n";
        }

        // 各項目
        for (int i = 0; i < MENU_COUNT; i++)
        {
            bool selected = (i == g_SelectedItem);
            const char* marker = selected ? "> " : "  ";

            ss << marker << MENU_LABELS[i];

            // 値表示
            if (i <= MENU_SE_VOLUME)
            {
                float value = GetCurrentValue(i);
                float minVal = (i == MENU_SENSITIVITY) ? SettingsRange::SENSITIVITY_MIN : SettingsRange::VOLUME_MIN;
                float maxVal = (i == MENU_SENSITIVITY) ? SettingsRange::SENSITIVITY_MAX : SettingsRange::VOLUME_MAX;
                int percent = static_cast<int>((value - minVal) / (maxVal - minVal) * 100.0f + 0.5f);

                // パディング
                int labelLen = static_cast<int>(strlen(MENU_LABELS[i]));
                for (int p = labelLen; p < 25; p++) ss << " ";

                ss << percent << "%";
            }

            ss << "\n\n";
        }

        ss << "\n\n";
        ss << "  [W/S] Select  [A/D] Adjust  [Enter] Confirm  [ESC] Back\n";

        g_pDebugText->SetText(ss.str().c_str());
        g_pDebugText->Draw();
        g_pDebugText->Clear();
    }

    Direct3D_DepthStencilStateDepthIsEnable(true);
}

//======================================
// シャドウ描画
//======================================
void Settings_DrawShadow()
{
    // 設定画面ではシャドウ不要
}