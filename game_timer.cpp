/****************************************
 * @file game_timer.cpp
 * @brief ゲームタイマー管理の実装
 * @author Natsume Shidara
 * @date 2026/01/12
 ****************************************/

#include "game_timer.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"

#include <algorithm>
#include <cmath>
#include <DirectXMath.h>
#include <filesystem>

using namespace DirectX;

//======================================
// 定数
//======================================
namespace
{
    // テクスチャパス（スコアと共通）
    constexpr const wchar_t* TIMER_TEXTURE_PATH = L"assets/ui/numbers.png";

    // フォントサイズ（テクスチャ内）
    constexpr int FONT_SRC_WIDTH = 89;
    constexpr int FONT_SRC_HEIGHT = 123;

    // 描画スケール（1/4サイズで表示）
    constexpr float DRAW_SCALE = 0.25f;

    // 追加の表示倍率
    constexpr float TIMER_SCALE = 1.5f;

    // 警告時間（残り30秒以下で赤く点滅）
    constexpr float WARNING_TIME = 30.0f;
    constexpr float BLINK_SPEED = 4.0f;

    // 表示位置（画面上部中央）
    constexpr float MARGIN_Y = 20.0f;
}

//======================================
// 内部変数
//======================================
static int g_TimerTexId = -1;
static float g_TotalTime = 180.0f;
static float g_RemainingTime = 180.0f;
static bool g_IsPaused = false;
static float g_BlinkTimer = 0.0f;

//======================================
// 内部関数
//======================================
static void DrawNumber(float x, float y, int num, float scale, const XMFLOAT4& color)
{
    if (g_TimerTexId < 0) return;

    // 描画サイズ（1/4スケール適用）
    float drawW = FONT_SRC_WIDTH * DRAW_SCALE * scale;
    float drawH = FONT_SRC_HEIGHT * DRAW_SCALE * scale;

    // 描画（フルスペック版）
    // 引数順序: texid, x, y, uvX, uvY, uvW, uvH, drawW, drawH, angle, color
    Sprite_Draw(
        g_TimerTexId,
        x, y,
        static_cast<float>(FONT_SRC_WIDTH * num), 0.0f,
        static_cast<float>(FONT_SRC_WIDTH),
        static_cast<float>(FONT_SRC_HEIGHT),
        drawW, drawH,
        0.0f,
        color
    );
}

//======================================
// 初期化
//======================================
void GameTimer_Initialize(float totalTime)
{
    g_TimerTexId = Texture_Load(TIMER_TEXTURE_PATH);
    g_TotalTime = totalTime;
    g_RemainingTime = totalTime;
    g_IsPaused = false;
    g_BlinkTimer = 0.0f;
}

//======================================
// 終了処理
//======================================
void GameTimer_Finalize()
{
    g_TimerTexId = -1;
}

//======================================
// 更新
//======================================
void GameTimer_Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);

    if (!g_IsPaused && g_RemainingTime > 0.0f)
    {
        g_RemainingTime -= dt;
        g_RemainingTime = std::fmaxf(0.0f, g_RemainingTime);
    }

    // 点滅タイマー
    g_BlinkTimer += dt * BLINK_SPEED;
}

//======================================
// 描画
//======================================
void GameTimer_Draw()
{
    if (g_TimerTexId < 0) return;

    // 画面サイズ取得
    float screenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());

    // 残り秒数を計算
    int seconds = static_cast<int>(std::ceil(g_RemainingTime));
    seconds = std::fmaxf(0, std::fminf(999, seconds));

    // 桁数を決定（最大3桁）
    int digit = 3;
    float charWidth = FONT_SRC_WIDTH * DRAW_SCALE * TIMER_SCALE;
    float totalWidth = charWidth * digit;

    // 中央配置
    float startX = (screenWidth - totalWidth) * 0.5f;
    float posY = MARGIN_Y;

    // 色を決定
    XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

    // 警告時間（残り30秒以下で赤く点滅）
    if (g_RemainingTime <= WARNING_TIME && g_RemainingTime > 0.0f)
    {
        float blink = (sinf(g_BlinkTimer) + 1.0f) * 0.5f;
        color.x = 1.0f;
        color.y = blink * 0.5f;
        color.z = blink * 0.5f;
    }

    // タイムアップ時は赤
    if (g_RemainingTime <= 0.0f)
    {
        color = { 1.0f, 0.2f, 0.2f, 1.0f };
    }

    Sprite_Begin();

    // 各桁を描画（左から）
    int temp = seconds;
    for (int i = digit - 1; i >= 0; i--)
    {
        int n = temp % 10;
        float x = startX + i * charWidth;
        DrawNumber(x, posY, n, TIMER_SCALE, color);
        temp /= 10;
    }
}

//======================================
// 残り時間取得
//======================================
float GameTimer_GetRemaining()
{
    return g_RemainingTime;
}

//======================================
// タイムアップ判定
//======================================
bool GameTimer_IsTimeUp()
{
    return g_RemainingTime <= 0.0f;
}

//======================================
// 一時停止
//======================================
void GameTimer_Pause()
{
    g_IsPaused = true;
}

//======================================
// 再開
//======================================
void GameTimer_Resume()
{
    g_IsPaused = false;
}

//======================================
// リセット
//======================================
void GameTimer_Reset(float totalTime)
{
    g_TotalTime = totalTime;
    g_RemainingTime = totalTime;
    g_IsPaused = false;
}