/****************************************
 * @file score.cpp
 * @brief スコア管理の実装
 * @author Natsume Shidara
 * @date 2025/07/09
 * @update 2026/01/12 - 立体機動アクションゲーム用に改造
 ****************************************/

#include "score.h"
#include "texture.h"
#include "sprite.h"

#include <algorithm>
#include <DirectXMath.h>
#include <filesystem>

using namespace DirectX;

//======================================
// 定数
//======================================
namespace
{
    // テクスチャパス
    constexpr const wchar_t* SCORE_TEXTURE_PATH = L"assets/ui/numbers.png";

    // フォントサイズ（テクスチャ内）
    constexpr int FONT_SRC_WIDTH = 89;
    constexpr int FONT_SRC_HEIGHT = 123;

    // 描画スケール（1/4サイズで表示）
    constexpr float DRAW_SCALE = 0.25f;

    // カウントアップ速度
    constexpr unsigned int COUNT_UP_SPEED = 10;
}

//======================================
// 内部変数
//======================================
static unsigned int g_Score = 0;           // 実際のスコア
static unsigned int g_ViewScore = 0;       // 表示用スコア（カウントアップ演出）
static unsigned int g_FinalScore = 0;      // 最終スコア（リザルト用）
static int g_Digit = 7;                    // 表示桁数
static unsigned int g_CounterStop = 9999999;  // 最大値
static float g_PosX = 0.0f;
static float g_PosY = 0.0f;
static int g_ScoreTexId = -1;

//======================================
// 内部関数
//======================================
static void DrawNumber(float x, float y, int num, float sizeX, float sizeY)
{
    if (g_ScoreTexId < 0) return;

    // 描画サイズ（1/4スケール適用）
    float drawW = FONT_SRC_WIDTH * DRAW_SCALE * sizeX;
    float drawH = FONT_SRC_HEIGHT * DRAW_SCALE * sizeY;

    // 描画（フルスペック版）
    // 引数順序: texid, x, y, uvX, uvY, uvW, uvH, drawW, drawH, angle, color
    Sprite_Draw(
        g_ScoreTexId,
        x, y,
        static_cast<float>(FONT_SRC_WIDTH * num), 0.0f,  // UV切り抜き位置
        static_cast<float>(FONT_SRC_WIDTH),               // UV幅
        static_cast<float>(FONT_SRC_HEIGHT),              // UV高さ
        drawW,                                            // 描画幅
        drawH,                                            // 描画高さ
        0.0f,                                             // 角度
        XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)                 // 色
    );
}

//======================================
// 初期化
//======================================
void Score_Initialize(float x, float y, int digit)
{
    g_Score = 0;
    g_ViewScore = 0;
    g_Digit = digit;
    g_PosX = x;
    g_PosY = y;

    g_ScoreTexId = Texture_Load(SCORE_TEXTURE_PATH);

    // カウンターストップ値を計算
    g_CounterStop = 1;
    for (int i = 0; i < digit; i++)
    {
        g_CounterStop *= 10;
    }
    g_CounterStop--;
}

//======================================
// 終了処理
//======================================
void Score_Finalize()
{
    g_ScoreTexId = -1;
}

//======================================
// 更新
//======================================
void Score_Update(double elapsed_time)
{
    (void)elapsed_time;

    // カウントアップ演出
    if (g_ViewScore < g_Score)
    {
        unsigned int diff = g_Score - g_ViewScore;
        unsigned int add = std::fmaxf(1u, diff / 10);
        add = std::fminf(add, COUNT_UP_SPEED);
        g_ViewScore = std::fminf(g_ViewScore + add, g_Score);
    }
}

//======================================
// 描画（位置・サイズ指定）
//======================================
void Score_Draw(float x, float y, float sizeX, float sizeY)
{
    if (g_ScoreTexId < 0) return;

    Sprite_Begin();

    unsigned int temp = std::fminf(g_ViewScore, g_CounterStop);

    // 1文字の描画幅（DRAW_SCALE適用）
    float charWidth = FONT_SRC_WIDTH * DRAW_SCALE * sizeX;

    for (int i = 0; i < g_Digit; i++)
    {
        int n = temp % 10;

        // 右から描画（右詰め）
        float drawX = x + (g_Digit - 1 - i) * charWidth;

        DrawNumber(drawX, y, n, sizeX, sizeY);
        temp /= 10;
    }
}

//======================================
// 描画（固定位置）
//======================================
void Score_Draw()
{
    Score_Draw(g_PosX, g_PosY, 1.0f, 1.0f);
}

//======================================
// スコア取得
//======================================
unsigned int Score_GetScore()
{
    return g_Score;
}

//======================================
// スコア加算
//======================================
void Score_AddScore(int value)
{
    g_Score = std::fminf(g_Score + static_cast<unsigned int>(value), g_CounterStop);
}

//======================================
// スコアリセット
//======================================
void Score_ResetScore()
{
    g_Score = 0;
    g_ViewScore = 0;
}

//======================================
// 最終スコア保存（ゲーム終了時）
//======================================
void Score_SaveFinal()
{
    g_FinalScore = g_Score;
}

//======================================
// 最終スコア取得
//======================================
unsigned int Score_GetFinalScore()
{
    return g_FinalScore;
}

//======================================
// リザルト画面用初期化（スコアをリセットしない）
//======================================
void Score_InitializeForResult(float x, float y, int digit)
{
    g_Digit = digit;
    g_PosX = x;
    g_PosY = y;

    // 最終スコアを表示用にセット
    g_Score = g_FinalScore;
    g_ViewScore = 0;  // 0からカウントアップ演出

    // テクスチャ読み込み
    g_ScoreTexId = Texture_Load(SCORE_TEXTURE_PATH);

    // カウンターストップ値を計算
    g_CounterStop = 1;
    for (int i = 0; i < digit; i++)
    {
        g_CounterStop *= 10;
    }
    g_CounterStop--;
}