/****************************************
 * @file result.cpp
 * @brief リザルト画面の実装
 * @author Natsume Shidara
 * @date 2025/08/19
 * @update 2026/01/12 - 立体機動アクションゲーム用に改造
 * @update 2026/01/13 - サウンド対応
 ****************************************/

#include "result.h"
#include "scene.h"
#include "score.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
#include "key_logger.h"
#include "pad_logger.h"
#include "fade.h"
#include "sound_manager.h"

#include <DirectXMath.h>

using namespace DirectX;

//======================================
// 定数
//======================================
namespace
{
    constexpr const wchar_t* RESULT_BG_TEXTURE_PATH = L"assets/ui/result_bg.png";
    constexpr const wchar_t* RESULT_TITLE_TEXTURE_PATH = L"assets/ui/result_title.png";
    constexpr const wchar_t* PRESS_START_TEXTURE_PATH = L"assets/ui/press_start.png";

    constexpr float SCORE_SIZE_MAX = 2.5f;
    constexpr float SCORE_SIZE_SPEED = 0.03f;
    constexpr int SCORE_DIGITS = 7;

    constexpr int FONT_SRC_WIDTH = 89;
    constexpr int FONT_SRC_HEIGHT = 123;
    constexpr float DRAW_SCALE = 0.25f;

    constexpr float BLINK_SPEED = 3.0f;

    constexpr float TITLE_Y_RATIO = 0.15f;
    constexpr float SCORE_Y_RATIO = 0.40f;
    constexpr float PRESS_ENTER_Y_RATIO = 0.75f;

    // スコアSE再生間隔
    constexpr float SCORE_SOUND_INTERVAL = 0.05f;
}

//======================================
// 内部変数
//======================================
static int g_ResultBGTexId = -1;
static int g_ResultTitleTexId = -1;
static int g_PressEnterTexId = -1;

static float g_ScreenWidth = 0.0f;
static float g_ScreenHeight = 0.0f;

static float g_ScoreScale = 0.0f;
static float g_BlinkTimer = 0.0f;
static bool g_CanPressEnter = false;

static float g_ScoreSoundTimer = 0.0f;
static bool g_ScoreFinishSoundPlayed = false;

//======================================
// 初期化
//======================================
void Result_Initialize()
{
    Fade_Start(1.0, false);

    g_ScreenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    g_ScreenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());

    g_ResultBGTexId = Texture_Load(RESULT_BG_TEXTURE_PATH);
    g_ResultTitleTexId = Texture_Load(RESULT_TITLE_TEXTURE_PATH);
    g_PressEnterTexId = Texture_Load(PRESS_START_TEXTURE_PATH);

    Score_InitializeForResult(0.0f, 0.0f, SCORE_DIGITS);

    g_ScoreScale = 0.0f;
    g_BlinkTimer = 0.0f;
    g_CanPressEnter = false;
    g_ScoreSoundTimer = 0.0f;
    g_ScoreFinishSoundPlayed = false;

    // リザルトBGM再生
    SoundManager_PlayBGM(SOUND_BGM_RESULT);
}

//======================================
// 終了処理
//======================================
void Result_Finalize()
{
    Score_Finalize();
    g_ResultBGTexId = -1;
    g_ResultTitleTexId = -1;
    g_PressEnterTexId = -1;
}

//======================================
// 更新
//======================================
void Result_Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);

    // スコア表示のスケールアップ演出
    if (g_ScoreScale < SCORE_SIZE_MAX)
    {
        g_ScoreScale += SCORE_SIZE_SPEED;

        // スコアカウントアップ中のSE
        g_ScoreSoundTimer += dt;
        if (g_ScoreSoundTimer >= SCORE_SOUND_INTERVAL)
        {
            SoundManager_PlaySE(SOUND_SE_SCORE_COUNT);
            g_ScoreSoundTimer = 0.0f;
        }

        if (g_ScoreScale >= SCORE_SIZE_MAX)
        {
            g_ScoreScale = SCORE_SIZE_MAX;
            g_CanPressEnter = true;

            // スコア表示完了SE
            if (!g_ScoreFinishSoundPlayed)
            {
                SoundManager_PlaySE(SOUND_SE_SCORE_FINISH);
                g_ScoreFinishSoundPlayed = true;
            }
        }
    }

    g_BlinkTimer += dt * BLINK_SPEED;

    Score_Update(elapsed_time);

    bool confirmInput = KeyLogger_IsTrigger(KK_ENTER) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_START) ||
        PadLogger_IsTrigger(0, XINPUT_GAMEPAD_A);

    if (g_CanPressEnter && confirmInput)
    {
        // 決定SE
        SoundManager_PlaySE(SOUND_SE_DECIDE);
        Fade_Start(1.0, true);
    }

    if (Fade_GetState() == FADE_STATE::FINISHED_OUT)
    {
        Scene_Change(Scene::TITLE);
    }
}

//======================================
// 描画
//======================================
void Result_Draw()
{
    Direct3D_DepthStencilStateDepthIsEnable(false);
    Direct3D_SetBlendState(BlendMode::Alpha);

    Sprite_Begin();

    // 背景
    if (g_ResultBGTexId >= 0)
    {
        Sprite_Draw(g_ResultBGTexId, 0.0f, 0.0f, g_ScreenWidth, g_ScreenHeight);
    }

    // タイトル
    if (g_ResultTitleTexId >= 0)
    {
        float titleW = static_cast<float>(Texture_Width(g_ResultTitleTexId));
        float titleH = static_cast<float>(Texture_Height(g_ResultTitleTexId));
        float titleX = (g_ScreenWidth - titleW) * 0.5f;
        float titleY = g_ScreenHeight * TITLE_Y_RATIO;

        Sprite_Draw(g_ResultTitleTexId, titleX, titleY);
    }

    // スコア表示
    if (g_ScoreScale > 0.0f)
    {
        float scoreCharWidth = FONT_SRC_WIDTH * DRAW_SCALE * g_ScoreScale;
        float scoreTotalWidth = scoreCharWidth * SCORE_DIGITS;
        float scoreHeight = FONT_SRC_HEIGHT * DRAW_SCALE * g_ScoreScale;

        float scoreX = (g_ScreenWidth - scoreTotalWidth) * 0.5f;
        float scoreY = g_ScreenHeight * SCORE_Y_RATIO - scoreHeight * 0.5f;

        Score_Draw(scoreX, scoreY, g_ScoreScale, g_ScoreScale);
    }

    // Press Enter
    if (g_CanPressEnter && g_PressEnterTexId >= 0)
    {
        float pressW = static_cast<float>(Texture_Width(g_PressEnterTexId));
        float pressH = static_cast<float>(Texture_Height(g_PressEnterTexId));
        float pressX = (g_ScreenWidth - pressW) * 0.5f;
        float pressY = g_ScreenHeight * PRESS_ENTER_Y_RATIO;

        float alpha = (sinf(g_BlinkTimer) + 1.0f) * 0.5f;
        alpha = 0.3f + alpha * 0.7f;
        XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, alpha };
        Sprite_Draw(g_PressEnterTexId, pressX, pressY, 0.0f, color);
    }

    Direct3D_DepthStencilStateDepthIsEnable(true);
}

//======================================
// シャドウ描画（空実装）
//======================================
void Result_DrawShadow()
{
}