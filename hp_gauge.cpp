/****************************************
 * @file hp_gauge.cpp
 * @brief HPゲージ管理の実装
 * @author Natsume Shidara
 * @date 2025/08/15
 * @update 2026/01/12 - 立体機動アクションゲーム用に改造
 ****************************************/

#include "hp_gauge.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
#include "player.h"

#include <algorithm>
#include <cmath>
#include <DirectXMath.h>

using namespace DirectX;

//======================================
// 定数
//======================================
namespace
{
    // テクスチャパス
    constexpr const wchar_t* HEART_TEXTURE_PATH = L"assets/ui/heart.png";

    // ハートテクスチャのUV設定（横に3つ並んでいる: 空、半分、満）
    constexpr int HEART_SRC_WIDTH = 12;
    constexpr int HEART_SRC_HEIGHT = 12;

    // 描画サイズ
    constexpr float HEART_DRAW_WIDTH = 48.0f;
    constexpr float HEART_DRAW_HEIGHT = 48.0f;

    // 表示位置（画面左下）
    constexpr float MARGIN_X = 20.0f;
    constexpr float MARGIN_Y = 20.0f;

    // アニメーション
    constexpr float DAMAGE_FLASH_DURATION = 0.5f;
    constexpr float LOW_HP_PULSE_SPEED = 4.0f;
    constexpr int LOW_HP_THRESHOLD = 3;
}

//======================================
// 内部変数
//======================================
static int g_HeartTexId = -1;
static XMFLOAT2 g_HpGaugePos = { 0.0f, 0.0f };

// アニメーション用
static int g_PrevHP = 0;
static float g_DamageFlashTimer = 0.0f;
static float g_PulseTimer = 0.0f;

//======================================
// 初期化
//======================================
void HpGauge_Initialize()
{
    g_HeartTexId = Texture_Load(HEART_TEXTURE_PATH);

    // 画面サイズから位置を計算（左下）
    float screenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());
    g_HpGaugePos.x = MARGIN_X;
    g_HpGaugePos.y = screenHeight - MARGIN_Y - HEART_DRAW_HEIGHT;

    g_PrevHP = Player_GetHP();
    g_DamageFlashTimer = 0.0f;
    g_PulseTimer = 0.0f;
}

//======================================
// 終了処理
//======================================
void HpGauge_Finalize()
{
    // テクスチャはTexture_AllRelease()で解放
    g_HeartTexId = -1;
}

//======================================
// 更新
//======================================
void HpGauge_Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);
    int currentHP = Player_GetHP();

    // ダメージ検出
    if (currentHP < g_PrevHP)
    {
        g_DamageFlashTimer = DAMAGE_FLASH_DURATION;
    }
    g_PrevHP = currentHP;

    // タイマー更新
    if (g_DamageFlashTimer > 0.0f)
    {
        g_DamageFlashTimer -= dt;
    }

    // 脈動タイマー
    g_PulseTimer += dt * LOW_HP_PULSE_SPEED;
}

//======================================
// 描画
//======================================
void HpGauge_Draw()
{
    if (g_HeartTexId < 0) return;

    // 現在HPと最大HPを取得
    int currentHP = Player_GetHP();
    int maxHP = Player_GetMaxHP();

    // ハートの数（1ハート = 2HP）
    int heartCount = (maxHP + 1) / 2;

    // 描画色
    XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

    // ダメージフラッシュ
    if (g_DamageFlashTimer > 0.0f)
    {
        float flash = g_DamageFlashTimer / DAMAGE_FLASH_DURATION;
        color.y = 1.0f - flash * 0.5f;
        color.z = 1.0f - flash * 0.5f;
    }

    // 低HP時の脈動
    float scale = 1.0f;
    if (currentHP <= LOW_HP_THRESHOLD && currentHP > 0)
    {
        scale = 1.0f + sinf(g_PulseTimer) * 0.1f;
    }

    Sprite_Begin();

    for (int i = 0; i < heartCount; i++)
    {
        // このハートが表すHP範囲 [2*i, 2*i+2)
        int hpSegment = currentHP - (i * 2);

        // UV切り抜き位置を決定
        float srcX = 0.0f;
        if (hpSegment >= 2)
        {
            // 2以上残っている → 満タンハート
            srcX = static_cast<float>(HEART_SRC_WIDTH * 2);
        }
        else if (hpSegment == 1)
        {
            // ちょうど1残っている → 半分ハート
            srcX = static_cast<float>(HEART_SRC_WIDTH);
        }
        else
        {
            // 0以下 → 空ハート
            srcX = 0.0f;
        }

        // 描画位置
        float drawX = g_HpGaugePos.x + i * (HEART_DRAW_WIDTH + 4.0f);
        float drawY = g_HpGaugePos.y;

        // 低HP時のスケール適用
        float drawW = HEART_DRAW_WIDTH * scale;
        float drawH = HEART_DRAW_HEIGHT * scale;

        // スケール時は中心を維持
        if (scale != 1.0f)
        {
            drawX -= (drawW - HEART_DRAW_WIDTH) * 0.5f;
            drawY -= (drawH - HEART_DRAW_HEIGHT) * 0.5f;
        }

        // 描画（フルスペック版）
        // 引数順序: texid, x, y, uvX, uvY, uvW, uvH, drawW, drawH, angle, color
        Sprite_Draw(
            g_HeartTexId,
            drawX, drawY,
            srcX, 0.0f,                                    // UV切り抜き位置
            static_cast<float>(HEART_SRC_WIDTH),           // UV幅
            static_cast<float>(HEART_SRC_HEIGHT),          // UV高さ
            drawW, drawH,                                  // 描画サイズ
            0.0f,                                          // 角度
            color                                          // 色
        );
    }
}