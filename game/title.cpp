/****************************************
 * @file title.cpp
 * @brief タイトル画面「斬空 -ZANKUU-」の実装
 * @author Natsume Shidara
 * @date 2026/01/12
 * @update 2026/01/13 - サウンド対応
 *
 * 演出フロー:
 * 1. 背景フェードイン (0.0s〜1.0s)
 * 2. 斬撃エフェクト (1.0s〜1.8s)
 * 3. ロゴ登場 (1.3s〜2.3s)
 * 4. Press Enter点滅 (2.5s〜)
 ****************************************/

#include "title.h"
#include "scene.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
#include "key_logger.h"
#include "pad_logger.h"
#include "fade.h"
#include "sound_manager.h"

#include <DirectXMath.h>
#include <cmath>
#include <algorithm>

using namespace DirectX;

//======================================
// 定数
//======================================
namespace
{
    constexpr const wchar_t* BG_TEXTURE_PATH = L"assets/ui/title_bg_only.png";
    constexpr const wchar_t* LOGO_TEXTURE_PATH = L"assets/ui/title_logo.png";
    constexpr const wchar_t* PRESS_START_TEXTURE_PATH = L"assets/ui/press_start.png";
    constexpr const wchar_t* WHITE_TEXTURE_PATH = L"assets/white.png";

    constexpr float TIME_BG_FADE_START = 0.0f;
    constexpr float TIME_BG_FADE_END = 1.0f;
    constexpr float TIME_SLASH_START = 1.0f;
    constexpr float TIME_SLASH_END = 1.8f;
    constexpr float TIME_LOGO_START = 1.3f;
    constexpr float TIME_LOGO_END = 2.3f;
    constexpr float TIME_PRESS_ENTER_START = 2.5f;

    constexpr float LOGO_SCALE_START = 1.5f;
    constexpr float LOGO_SCALE_END = 1.0f;

    constexpr float BLINK_SPEED = 3.0f;

    constexpr int SLASH_COUNT = 3;
    constexpr float SLASH_DURATION = 0.15f;
}

//======================================
// 斬撃エフェクト構造体
//======================================
struct SlashEffect
{
    float startTime;
    float startX, startY;
    float endX, endY;
    float angle;
    float width;
    float length;
    bool active;
    bool soundPlayed;  // SE再生済みフラグ
};

//======================================
// 内部変数
//======================================
static int g_BgTexId = -1;
static int g_LogoTexId = -1;
static int g_PressEnterTexId = -1;
static int g_WhiteTexId = -1;

static float g_ScreenWidth = 0.0f;
static float g_ScreenHeight = 0.0f;

static float g_Timer = 0.0f;
static float g_BlinkTimer = 0.0f;
static bool g_CanInput = false;
static bool g_IsTransitioning = false;
static Scene g_NextScene = Scene::GAME;  // 遷移先シーン

static SlashEffect g_Slashes[SLASH_COUNT];

//======================================
// 内部関数プロトタイプ
//======================================
static void InitSlashEffects();
static void UpdateSlashEffects(float dt);
static void DrawSlashEffects();
static float EaseOutCubic(float t);
static float EaseOutBack(float t);

//======================================
// イージング関数
//======================================
static float EaseOutCubic(float t)
{
    return 1.0f - powf(1.0f - t, 3.0f);
}

static float EaseOutBack(float t)
{
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
}

//======================================
// 斬撃エフェクト初期化
//======================================
static void InitSlashEffects()
{
    g_Slashes[0].startTime = TIME_SLASH_START;
    g_Slashes[0].startX = -200.0f;
    g_Slashes[0].startY = g_ScreenHeight * 0.2f;
    g_Slashes[0].endX = g_ScreenWidth + 200.0f;
    g_Slashes[0].endY = g_ScreenHeight * 0.4f;
    g_Slashes[0].angle = -10.0f * (3.14159f / 180.0f);
    g_Slashes[0].width = 8.0f;
    g_Slashes[0].length = 400.0f;
    g_Slashes[0].active = true;
    g_Slashes[0].soundPlayed = false;

    g_Slashes[1].startTime = TIME_SLASH_START + 0.1f;
    g_Slashes[1].startX = g_ScreenWidth + 200.0f;
    g_Slashes[1].startY = g_ScreenHeight * 0.3f;
    g_Slashes[1].endX = -200.0f;
    g_Slashes[1].endY = g_ScreenHeight * 0.6f;
    g_Slashes[1].angle = 15.0f * (3.14159f / 180.0f);
    g_Slashes[1].width = 6.0f;
    g_Slashes[1].length = 350.0f;
    g_Slashes[1].active = true;
    g_Slashes[1].soundPlayed = false;

    g_Slashes[2].startTime = TIME_SLASH_START + 0.2f;
    g_Slashes[2].startX = -200.0f;
    g_Slashes[2].startY = g_ScreenHeight * 0.5f;
    g_Slashes[2].endX = g_ScreenWidth + 200.0f;
    g_Slashes[2].endY = g_ScreenHeight * 0.48f;
    g_Slashes[2].angle = -2.0f * (3.14159f / 180.0f);
    g_Slashes[2].width = 10.0f;
    g_Slashes[2].length = 500.0f;
    g_Slashes[2].active = true;
    g_Slashes[2].soundPlayed = false;
}

//======================================
// 斬撃エフェクト更新（SE再生）
//======================================
static void UpdateSlashEffects(float dt)
{
    (void)dt;

    for (int i = 0; i < SLASH_COUNT; i++)
    {
        SlashEffect& slash = g_Slashes[i];
        if (!slash.active) continue;

        // 斬撃開始タイミングでSE再生
        if (!slash.soundPlayed && g_Timer >= slash.startTime)
        {
            SoundManager_PlaySE(SOUND_SE_TITLE_SLASH);
            slash.soundPlayed = true;
        }
    }
}

//======================================
// 斬撃エフェクト描画
//======================================
static void DrawSlashEffects()
{
    if (g_WhiteTexId < 0) return;

    for (int i = 0; i < SLASH_COUNT; i++)
    {
        SlashEffect& slash = g_Slashes[i];
        if (!slash.active) continue;

        float elapsed = g_Timer - slash.startTime;
        if (elapsed < 0.0f || elapsed > SLASH_DURATION) continue;

        float progress = elapsed / SLASH_DURATION;
        float easedProgress = EaseOutCubic(progress);

        float currentX = slash.startX + (slash.endX - slash.startX) * easedProgress;
        float currentY = slash.startY + (slash.endY - slash.startY) * easedProgress;

        float alpha = 1.0f;
        if (progress > 0.5f)
        {
            alpha = 1.0f - (progress - 0.5f) * 2.0f;
        }

        for (int g = 3; g >= 0; g--)
        {
            float glowAlpha = alpha * (0.3f - g * 0.08f);
            float glowWidth = slash.width + g * 8.0f;

            XMFLOAT4 color = { 0.7f, 0.85f, 1.0f, glowAlpha };

            Sprite_Draw(
                g_WhiteTexId,
                currentX - slash.length * 0.5f,
                currentY - glowWidth * 0.5f,
                0.0f, 0.0f, 1.0f, 1.0f,
                slash.length, glowWidth,
                slash.angle,
                color
            );
        }

        XMFLOAT4 coreColor = { 1.0f, 1.0f, 1.0f, alpha };
        Sprite_Draw(
            g_WhiteTexId,
            currentX - slash.length * 0.5f,
            currentY - slash.width * 0.25f,
            0.0f, 0.0f, 1.0f, 1.0f,
            slash.length, slash.width * 0.5f,
            slash.angle,
            coreColor
        );
    }
}

//======================================
// 初期化
//======================================
void Title_Initialize()
{
    g_ScreenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    g_ScreenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());

    g_BgTexId = Texture_Load(BG_TEXTURE_PATH);
    g_LogoTexId = Texture_Load(LOGO_TEXTURE_PATH);
    g_PressEnterTexId = Texture_Load(PRESS_START_TEXTURE_PATH);
    g_WhiteTexId = Texture_Load(WHITE_TEXTURE_PATH);

    g_Timer = 0.0f;
    g_BlinkTimer = 0.0f;
    g_CanInput = false;
    g_IsTransitioning = false;
    g_NextScene = Scene::GAME;

    InitSlashEffects();

    // タイトルBGM再生
    SoundManager_PlayBGM(SOUND_BGM_TITLE);

    if (Fade_GetState() == FADE_STATE::FINISHED_OUT)
    {
        Fade_Start(0.5, false);
    }
}

//======================================
// 終了処理
//======================================
void Title_Finalize()
{
    g_BgTexId = -1;
    g_LogoTexId = -1;
    g_PressEnterTexId = -1;
    g_WhiteTexId = -1;
}

//======================================
// 更新
//======================================
void Title_Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);

    g_Timer += dt;
    g_BlinkTimer += dt * BLINK_SPEED;

    // 斬撃エフェクト更新（SE再生）
    UpdateSlashEffects(dt);

    if (g_Timer >= TIME_PRESS_ENTER_START)
    {
        g_CanInput = true;
    }

    if (g_IsTransitioning)
    {
        if (Fade_GetState() == FADE_STATE::FINISHED_OUT)
        {
            Scene_Change(g_NextScene);
        }
        return;
    }

    // ゲーム開始入力
    bool startInput = KeyLogger_IsTrigger(KK_ENTER) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_START) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_A);

    // 設定画面入力
    bool settingsInput = KeyLogger_IsTrigger(KK_O) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_BACK) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_Y);

    if (g_CanInput && startInput)
    {
        // 決定SE
        SoundManager_PlaySE(SOUND_SE_DECIDE);
        g_NextScene = Scene::GAME;
        g_IsTransitioning = true;
        Fade_Start(1.0, true);
    }
    else if (g_CanInput && settingsInput)
    {
        // 設定画面へ
        SoundManager_PlaySE(SOUND_SE_SELECT);
        g_NextScene = Scene::SETTINGS;
        g_IsTransitioning = true;
        Fade_Start(0.5, true);
    }
}

//======================================
// 描画
//======================================
void Title_Draw()
{
    Direct3D_DepthStencilStateDepthIsEnable(false);
    Direct3D_SetBlendState(BlendMode::Alpha);

    Sprite_Begin();

    // 背景
    if (g_BgTexId >= 0)
    {
        float bgAlpha = 1.0f;
        if (g_Timer < TIME_BG_FADE_END)
        {
            float progress = (g_Timer - TIME_BG_FADE_START) / (TIME_BG_FADE_END - TIME_BG_FADE_START);
            bgAlpha = std::clamp(progress, 0.0f, 1.0f);
        }

        XMFLOAT4 bgColor = { 1.0f, 1.0f, 1.0f, bgAlpha };
        Sprite_Draw(g_BgTexId, 0.0f, 0.0f, g_ScreenWidth, g_ScreenHeight, 0.0f, bgColor);
    }

    // 斬撃エフェクト
    DrawSlashEffects();

    // ロゴ
    if (g_LogoTexId >= 0 && g_Timer >= TIME_LOGO_START)
    {
        float logoW = static_cast<float>(Texture_Width(g_LogoTexId));
        float logoH = static_cast<float>(Texture_Height(g_LogoTexId));

        float logoProgress = (g_Timer - TIME_LOGO_START) / (TIME_LOGO_END - TIME_LOGO_START);
        logoProgress = std::clamp(logoProgress, 0.0f, 1.0f);

        float easedProgress = EaseOutBack(logoProgress);
        float scale = LOGO_SCALE_START + (LOGO_SCALE_END - LOGO_SCALE_START) * easedProgress;
        float alpha = EaseOutCubic(logoProgress);

        float drawW = logoW * scale;
        float drawH = logoH * scale;
        float logoX = (g_ScreenWidth - drawW) * 0.5f;
        float logoY = (g_ScreenHeight - drawH) * 0.5f - 50.0f;

        XMFLOAT4 logoColor = { 1.0f, 1.0f, 1.0f, alpha };
        Sprite_Draw(g_LogoTexId, logoX, logoY, drawW, drawH, 0.0f, logoColor);
    }

    // Press Enter
    if (g_PressEnterTexId >= 0 && g_CanInput)
    {
        float pressW = static_cast<float>(Texture_Width(g_PressEnterTexId));
        float pressH = static_cast<float>(Texture_Height(g_PressEnterTexId));
        float pressX = (g_ScreenWidth - pressW) * 0.5f;
        float pressY = g_ScreenHeight * 0.8f;

        float blinkAlpha = (sinf(g_BlinkTimer) + 1.0f) * 0.5f;
        blinkAlpha = 0.3f + blinkAlpha * 0.7f;

        float fadeProgress = (g_Timer - TIME_PRESS_ENTER_START) / 0.5f;
        fadeProgress = std::clamp(fadeProgress, 0.0f, 1.0f);
        float alpha = blinkAlpha * fadeProgress;

        XMFLOAT4 pressColor = { 1.0f, 1.0f, 1.0f, alpha };
        Sprite_Draw(g_PressEnterTexId, pressX, pressY, 0.0f, pressColor);
    }

    Direct3D_DepthStencilStateDepthIsEnable(true);
}

//======================================
// シャドウ描画（空実装）
//======================================
void Title_DrawShadow()
{
}