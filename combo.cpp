/****************************************
 * @file combo.cpp
 * @brief コンボ管理システムの実装
 * @author Natsume Shidara
 * @date 2026/01/12
 * @update 2026/01/13 - サウンド対応
 ****************************************/

#include "combo.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
#include "sound_manager.h"

#include <algorithm>
#include <cmath>
#include <DirectXMath.h>

using namespace DirectX;

//======================================
// 定数
//======================================
namespace
{
    constexpr const wchar_t* COMBO_NUMBER_PATH = L"assets/ui/numbers.png";
    constexpr const wchar_t* COMBO_TEXT_PATH = L"assets/ui/combo_text.png";

    constexpr int FONT_SRC_WIDTH = 89;
    constexpr int FONT_SRC_HEIGHT = 123;

    constexpr float DRAW_SCALE = 0.5f;

    constexpr float COMBO_POS_X_RATIO = 0.85f;
    constexpr float COMBO_POS_Y_RATIO = 0.35f;

    constexpr float COMBO_TIMEOUT = 3.0f;

    constexpr float BASE_SCALE = 1.0f;
    constexpr float MAX_SCALE = 2.5f;
    constexpr float SCALE_PER_COMBO = 0.05f;

    constexpr float POP_SCALE_BOOST = 0.3f;
    constexpr float POP_DURATION = 0.15f;

    constexpr int SHAKE_THRESHOLD = 20;
    constexpr float SHAKE_INTENSITY = 3.0f;
    constexpr float SHAKE_SPEED = 50.0f;

    constexpr int TIER_1 = 5;
    constexpr int TIER_2 = 10;
    constexpr int TIER_3 = 20;
    constexpr int TIER_4 = 50;
    constexpr int TIER_5 = 100;

    constexpr float BONUS_BASE = 1.0f;
    constexpr float BONUS_PER_COMBO = 0.1f;
    constexpr float BONUS_MAX = 5.0f;
}

//======================================
// 内部変数
//======================================
static int g_ComboTexId = -1;
static int g_ComboTextTexId = -1;

static int g_ComboCount = 0;
static float g_ComboTimer = 0.0f;
static float g_PopTimer = 0.0f;
static float g_ShakeTimer = 0.0f;
static float g_DisplayTimer = 0.0f;

static int g_MaxCombo = 0;

//======================================
// 内部関数プロトタイプ
//======================================
static XMFLOAT4 GetComboColor(int combo);
static float GetComboScale(int combo);
static void DrawNumber(float x, float y, int num, float scale, const XMFLOAT4& color);

//======================================
// 色を取得（コンボ数に応じて変化）
//======================================
static XMFLOAT4 GetComboColor(int combo)
{
    if (combo < TIER_1)
    {
        return XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else if (combo < TIER_2)
    {
        return XMFLOAT4(1.0f, 1.0f, 0.3f, 1.0f);
    }
    else if (combo < TIER_3)
    {
        return XMFLOAT4(1.0f, 0.6f, 0.1f, 1.0f);
    }
    else if (combo < TIER_4)
    {
        return XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f);
    }
    else if (combo < TIER_5)
    {
        return XMFLOAT4(1.0f, 0.2f, 0.8f, 1.0f);
    }
    else
    {
        float hue = fmodf(g_DisplayTimer * 2.0f, 1.0f);

        float r, g, b;
        int i = static_cast<int>(hue * 6.0f);
        float f = hue * 6.0f - i;

        switch (i % 6)
        {
        case 0: r = 1.0f; g = f;        b = 0.0f;     break;
        case 1: r = 1.0f - f; g = 1.0f; b = 0.0f;     break;
        case 2: r = 0.0f; g = 1.0f;     b = f;        break;
        case 3: r = 0.0f; g = 1.0f - f; b = 1.0f;     break;
        case 4: r = f;    g = 0.0f;     b = 1.0f;     break;
        default: r = 1.0f; g = 0.0f;    b = 1.0f - f; break;
        }

        return XMFLOAT4(r, g, b, 1.0f);
    }
}

//======================================
// スケールを取得（コンボ数に応じて変化）
//======================================
static float GetComboScale(int combo)
{
    float scale = BASE_SCALE + combo * SCALE_PER_COMBO;
    return std::fminf(scale, MAX_SCALE);
}

//======================================
// 数字を描画
//======================================
static void DrawNumber(float x, float y, int num, float scale, const XMFLOAT4& color)
{
    if (g_ComboTexId < 0) return;

    float drawW = FONT_SRC_WIDTH * DRAW_SCALE * scale;
    float drawH = FONT_SRC_HEIGHT * DRAW_SCALE * scale;

    Sprite_Draw(
        g_ComboTexId,
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
void Combo_Initialize()
{
    g_ComboTexId = Texture_Load(COMBO_NUMBER_PATH);
    g_ComboTextTexId = Texture_Load(COMBO_TEXT_PATH);

    g_ComboCount = 0;
    g_ComboTimer = 0.0f;
    g_PopTimer = 0.0f;
    g_ShakeTimer = 0.0f;
    g_DisplayTimer = 0.0f;
    g_MaxCombo = 0;
}

//======================================
// 終了処理
//======================================
void Combo_Finalize()
{
    g_ComboTexId = -1;
    g_ComboTextTexId = -1;
}

//======================================
// 更新
//======================================
void Combo_Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);

    g_DisplayTimer += dt;
    g_ShakeTimer += dt * SHAKE_SPEED;

    if (g_PopTimer > 0.0f)
    {
        g_PopTimer -= dt;
    }

    if (g_ComboCount > 0)
    {
        g_ComboTimer -= dt;
        if (g_ComboTimer <= 0.0f)
        {
            g_ComboCount = 0;
            g_ComboTimer = 0.0f;
        }
    }
}

//======================================
// 描画
//======================================
void Combo_Draw()
{
    if (g_ComboCount <= 0) return;
    if (g_ComboTexId < 0) return;

    float screenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    float screenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());

    float baseX = screenWidth * COMBO_POS_X_RATIO;
    float baseY = screenHeight * COMBO_POS_Y_RATIO;

    float scale = GetComboScale(g_ComboCount);
    if (g_PopTimer > 0.0f)
    {
        float popProgress = g_PopTimer / POP_DURATION;
        scale += POP_SCALE_BOOST * popProgress;
    }

    XMFLOAT4 color = GetComboColor(g_ComboCount);

    if (g_ComboTimer < 1.0f)
    {
        color.w = g_ComboTimer;
    }

    float shakeX = 0.0f;
    float shakeY = 0.0f;
    if (g_ComboCount >= SHAKE_THRESHOLD)
    {
        float intensity = SHAKE_INTENSITY * (1.0f + (g_ComboCount - SHAKE_THRESHOLD) * 0.05f);
        shakeX = sinf(g_ShakeTimer) * intensity;
        shakeY = cosf(g_ShakeTimer * 1.3f) * intensity * 0.7f;
    }

    int temp = g_ComboCount;
    int digitCount = 0;
    while (temp > 0)
    {
        digitCount++;
        temp /= 10;
    }
    if (digitCount == 0) digitCount = 1;

    float charWidth = FONT_SRC_WIDTH * DRAW_SCALE * scale;
    float totalWidth = charWidth * digitCount;

    float startX = baseX - totalWidth + shakeX;
    float drawY = baseY + shakeY;

    Sprite_Begin();

    temp = g_ComboCount;
    for (int i = 0; i < digitCount; i++)
    {
        int n = temp % 10;
        float drawX = startX + (digitCount - 1 - i) * charWidth;

        if (g_ComboCount >= SHAKE_THRESHOLD)
        {
            drawX += sinf(g_ShakeTimer + i * 0.5f) * SHAKE_INTENSITY * 0.3f;
            drawY += cosf(g_ShakeTimer * 1.1f + i * 0.7f) * SHAKE_INTENSITY * 0.2f;
        }

        DrawNumber(drawX, drawY, n, scale, color);
        temp /= 10;
        drawY = baseY + shakeY;
    }

    if (g_ComboTextTexId >= 0)
    {
        float textScale = scale * 0.6f;
        float textX = baseX - totalWidth + shakeX;
        float textY = baseY + FONT_SRC_HEIGHT * DRAW_SCALE * scale + 5.0f + shakeY;

        if (g_ComboCount >= SHAKE_THRESHOLD)
        {
            textX += sinf(g_ShakeTimer * 0.8f) * SHAKE_INTENSITY * 0.5f;
            textY += cosf(g_ShakeTimer * 0.9f) * SHAKE_INTENSITY * 0.3f;
        }

        Sprite_Draw(g_ComboTextTexId, textX, textY, 0.0f, color);
    }
}

//======================================
// コンボ追加
//======================================
void Combo_Add(int count)
{
    g_ComboCount += count;
    g_ComboTimer = COMBO_TIMEOUT;
    g_PopTimer = POP_DURATION;

    if (g_ComboCount > g_MaxCombo)
    {
        g_MaxCombo = g_ComboCount;
    }

    // コンボ数に応じたSE再生
    //SoundManager_PlayComboSE(g_ComboCount);
}

//======================================
// コンボ数取得
//======================================
int Combo_GetCount()
{
    return g_ComboCount;
}

//======================================
// コンボリセット
//======================================
void Combo_Reset()
{
    g_ComboCount = 0;
    g_ComboTimer = 0.0f;
}

//======================================
// 最大コンボ数取得
//======================================
int Combo_GetMaxCombo()
{
    return g_MaxCombo;
}

//======================================
// ボーナススコア計算
//======================================
int Combo_GetBonusScore()
{
    float multiplier = BONUS_BASE + g_ComboCount * BONUS_PER_COMBO;
    multiplier = std::fminf(multiplier, BONUS_MAX);
    return static_cast<int>(100.0f * multiplier);
}