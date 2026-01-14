/****************************************
 * @file ui.cpp
 * @brief ゲームUI管理の実装
 * @detail HP表示などのUI要素を管理
 * @author Natsume Shidara
 * @date 2026/01/12
 ****************************************/

#include "ui.h"
#include "player.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"

 //======================================
 // 定数
 //======================================
namespace UIConfig
{
    // HP表示設定
    constexpr int MAX_HEARTS = 10;              // 最大ハート数
    constexpr float HEART_SIZE = 32.0f;         // ハートのサイズ
    constexpr float HEART_SPACING = 4.0f;       // ハート間の間隔
    constexpr float HEART_MARGIN_X = 20.0f;     // 画面左端からのマージン
    constexpr float HEART_MARGIN_Y = 20.0f;     // 画面下端からのマージン

    // テクスチャパス
    constexpr const wchar_t* HEART_FULL_PATH = L"assets/ui/heart_full.png";
    constexpr const wchar_t* HEART_EMPTY_PATH = L"assets/ui/heart_empty.png";

    // アニメーション
    constexpr float DAMAGE_FLASH_DURATION = 0.3f;  // ダメージ時のフラッシュ時間
    constexpr float HEART_PULSE_SPEED = 3.0f;      // 低HP時の脈動速度
    constexpr float HEART_PULSE_SCALE = 0.1f;      // 脈動の強さ
}

//======================================
// 内部変数
//======================================
namespace
{
    int g_TexHeartFull = -1;      // ハート（満）のテクスチャID
    int g_TexHeartEmpty = -1;     // ハート（空）のテクスチャID

    int g_PrevHP = 0;             // 前フレームのHP（ダメージ検出用）
    float g_DamageFlashTimer = 0.0f;  // ダメージフラッシュタイマー
    float g_PulseTimer = 0.0f;    // 脈動タイマー
}

//======================================
// 初期化
//======================================
void UI_Initialize()
{
    // テクスチャ読み込み
    g_TexHeartFull = Texture_Load(UIConfig::HEART_FULL_PATH);
    g_TexHeartEmpty = Texture_Load(UIConfig::HEART_EMPTY_PATH);

    // エラーチェック（デバッグ用）
    if (g_TexHeartFull < 0)
    {
        // フォールバック: 画像がない場合は何もしない（後でログ出力など）
    }

    // 初期HP取得
    g_PrevHP = Player_GetHP();
    g_DamageFlashTimer = 0.0f;
    g_PulseTimer = 0.0f;
}

//======================================
// 終了処理
//======================================
void UI_Finalize()
{
    // テクスチャはTexture_AllRelease()で一括解放されるため
    // 個別解放は不要
    g_TexHeartFull = -1;
    g_TexHeartEmpty = -1;
}

//======================================
// 更新
//======================================
void UI_Update(float dt)
{
    using namespace UIConfig;

    int currentHP = Player_GetHP();

    // ダメージ検出
    if (currentHP < g_PrevHP)
    {
        g_DamageFlashTimer = DAMAGE_FLASH_DURATION;
    }
    g_PrevHP = currentHP;

    // ダメージフラッシュタイマー更新
    if (g_DamageFlashTimer > 0.0f)
    {
        g_DamageFlashTimer -= dt;
    }

    // 脈動タイマー更新（低HP時）
    g_PulseTimer += dt * HEART_PULSE_SPEED;
}

//======================================
// 描画
//======================================
void UI_Draw()
{
    using namespace UIConfig;

    // テクスチャが読み込めていない場合はスキップ
    if (g_TexHeartFull < 0) return;

    // 画面サイズ取得
    float screenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    float screenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());

    // 現在のHP取得
    int currentHP = Player_GetHP();

    // 描画開始
    Sprite_Begin();

    // ハートの描画位置（画面左下）
    float baseX = HEART_MARGIN_X;
    float baseY = screenHeight - HEART_MARGIN_Y - HEART_SIZE;

    // 色の設定
    DirectX::XMFLOAT4 colorFull = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT4 colorEmpty = { 0.5f, 0.5f, 0.5f, 0.5f };

    // ダメージフラッシュ
    if (g_DamageFlashTimer > 0.0f)
    {
        float flashIntensity = g_DamageFlashTimer / DAMAGE_FLASH_DURATION;
        colorFull.x = 1.0f;
        colorFull.y = 1.0f - flashIntensity * 0.5f;
        colorFull.z = 1.0f - flashIntensity * 0.5f;
    }

    // 低HP時の脈動（HP3以下）
    float pulseScale = 1.0f;
    if (currentHP <= 3 && currentHP > 0)
    {
        pulseScale = 1.0f + sinf(g_PulseTimer) * HEART_PULSE_SCALE;
    }

    // 各ハートを描画
    for (int i = 0; i < MAX_HEARTS; i++)
    {
        float x = baseX + i * (HEART_SIZE + HEART_SPACING);
        float y = baseY;

        // サイズ（脈動適用）
        float size = HEART_SIZE;
        if (i < currentHP && currentHP <= 3)
        {
            size *= pulseScale;
            // 中心を維持するためにオフセット
            x -= (size - HEART_SIZE) * 0.5f;
            y -= (size - HEART_SIZE) * 0.5f;
        }

        if (i < currentHP)
        {
            // HPがある → 満ハート
            Sprite_Draw(g_TexHeartFull, x, y, size, size, 0.0f, colorFull);
        }
        else
        {
            // HPがない → 空ハート（存在する場合）
            if (g_TexHeartEmpty >= 0)
            {
                Sprite_Draw(g_TexHeartEmpty, x, y, size, size, 0.0f, colorEmpty);
            }
        }
    }
}