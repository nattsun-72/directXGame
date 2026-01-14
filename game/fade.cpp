/**
 * @file fade.cpp
 * @brief フェードイン・アウト制御
 * @author Natsume Shidara
 * @date 2025/07/10
 */

#include "fade.h"

#include <algorithm>

#include "debug_ostream.h"
#include "direct3d.h"
#include "sprite.h"
#include "texture.h"

static double g_FadeTime = 0.0;
static double g_FadeStartTime = 0.0;
static double g_AccumulatedTime = 0.0;
static auto g_State = FADE_STATE::NONE;
static int g_FadeTexId = -1;

static float g_Alpha = 0.0f;
static Color::COLOR g_color = Color::BLACK;

void Fade_Initialize()
{
    g_FadeTime = 1.0;
    g_FadeStartTime = 0.0;
    g_AccumulatedTime = 0.0;
    g_color = Color::BLACK;
    g_Alpha = 0.0f;
    g_State = FADE_STATE::NONE;

    g_FadeTexId = Texture_Load(L"assets/white.png");
}

void Fade_Finalize()
{}

void Fade_Update(double elapsed_time)
{
    if (g_State == FADE_STATE::FINISHED_IN || g_State == FADE_STATE::FINISHED_OUT)
        return;
    if (g_State == FADE_STATE::NONE)
        return;

    g_AccumulatedTime += elapsed_time;

    double progress = min((g_AccumulatedTime - g_FadeStartTime) / g_FadeTime, 1.0);

    if (progress >= 1)
    {
        g_State = g_State == FADE_STATE::FADE_IN ? FADE_STATE::FINISHED_IN : FADE_STATE::FINISHED_OUT;
    }

    g_Alpha = static_cast<float>(g_State == FADE_STATE::FADE_IN ? 1.0 - progress : progress);
}

void Fade_Draw()
{
    if (g_State == FADE_STATE::NONE)
        return;
    if (g_State == FADE_STATE::FINISHED_IN)
        return;

    Color::COLOR color = g_color;
    color.w = g_Alpha;

    float width = static_cast<float>(Direct3D_GetBackBufferWidth());
    float height = static_cast<float>(Direct3D_GetBackBufferHeight());
    Sprite_Draw(g_FadeTexId, 0.0f, 0.0f, width, height, 0.0f, color);
}

void Fade_Start(double duration, bool isFadeOut, Color::COLOR color)
{
    g_FadeStartTime = g_AccumulatedTime;
    g_FadeTime = duration;
    g_State = isFadeOut ? FADE_STATE::FADE_OUT : FADE_STATE::FADE_IN;
    g_color = color;
    g_Alpha = isFadeOut ? 1.0f : 0.0f;
}

FADE_STATE Fade_GetState()
{
    return g_State;
}
