#ifndef COLOR_ALIASES_H
#define COLOR_ALIASES_H

#include <DirectXMath.h>

namespace Color
{
    // 型定義
    using COLOR = DirectX::XMFLOAT4;

    inline COLOR SetOpacity(COLOR color, float o)
    {
        return { color.x, color.y, color.z, o };
    }

    ///////////////////////////////////////////
    // 基本色 / Basic Colors
    ///////////////////////////////////////////
    constexpr COLOR BLACK            = {0.0f, 0.0f, 0.0f, 1.0f};  // 黒
    constexpr COLOR WHITE            = {1.0f, 1.0f, 1.0f, 1.0f};  // 白
    constexpr COLOR GRAY             = {0.5f, 0.5f, 0.5f, 1.0f};  // グレー
    constexpr COLOR DARK_GRAY        = {0.25f, 0.25f, 0.25f, 1.0f};  // 濃いグレー
    constexpr COLOR LIGHT_GRAY       = {0.75f, 0.75f, 0.75f, 1.0f};  // 明るいグレー

    ///////////////////////////////////////////
    // 不透明／透明色 / Opaque & Transparent
    ///////////////////////////////////////////
    constexpr COLOR INVISIBLE_BLACK  = {0.0f, 0.0f, 0.0f, 0.0f};  // 透明黒
    constexpr COLOR INVISIBLE_WHITE  = {1.0f, 1.0f, 1.0f, 0.0f};  // 透明白
    constexpr COLOR TRANSLUCENT_RED  = {1.0f, 0.0f, 0.0f, 0.5f};  // 半透明赤
    constexpr COLOR TRANSLUCENT_GREEN= {0.0f, 1.0f, 0.0f, 0.5f};  // 半透明緑
    constexpr COLOR TRANSLUCENT_BLUE = {0.0f, 0.0f, 1.0f, 0.5f};  // 半透明青

    ///////////////////////////////////////////
    // 純色 / Primary & Secondary
    ///////////////////////////////////////////
    constexpr COLOR RED              = {1.0f, 0.0f, 0.0f, 1.0f};  // 赤
    constexpr COLOR GREEN            = {0.0f, 1.0f, 0.0f, 1.0f};  // 緑
    constexpr COLOR BLUE             = {0.0f, 0.0f, 1.0f, 1.0f};  // 青
    constexpr COLOR YELLOW           = {1.0f, 1.0f, 0.0f, 1.0f};  // 黄
    constexpr COLOR CYAN             = {0.0f, 1.0f, 1.0f, 1.0f};  // シアン
    constexpr COLOR MAGENTA          = {1.0f, 0.0f, 1.0f, 1.0f};  // マゼンタ
    constexpr COLOR ORANGE           = {1.0f, 0.5f, 0.0f, 1.0f};  // オレンジ
    constexpr COLOR PURPLE           = {0.5f, 0.0f, 0.5f, 1.0f};  // 紫
    constexpr COLOR BROWN            = {0.6f, 0.3f, 0.1f, 1.0f};  // ブラウン
    constexpr COLOR PINK             = {1.0f, 0.0f, 0.5f, 1.0f};  // ピンク

    ///////////////////////////////////////////
    // 暖色系 / Warm Colors
    ///////////////////////////////////////////
    constexpr COLOR CORAL            = {1.0f, 0.5f, 0.31f, 1.0f}; // コーラル
    constexpr COLOR SALMON           = {0.98f, 0.5f, 0.45f, 1.0f}; // サーモン
    constexpr COLOR LIME             = {0.75f, 1.0f, 0.0f, 1.0f};  // ライム
    constexpr COLOR GOLD             = {1.0f, 0.84f, 0.0f, 1.0f}; // ゴールド

    ///////////////////////////////////////////
    // 冷色系 / Cool Colors
    ///////////////////////////////////////////
    constexpr COLOR TEAL             = {0.0f, 0.5f, 0.5f, 1.0f};  // ティール
    constexpr COLOR NAVY             = {0.0f, 0.0f, 0.5f, 1.0f};  // ネイビー
    constexpr COLOR AQUA             = {0.0f, 1.0f, 1.0f, 1.0f};  // アクア
    constexpr COLOR SKY_BLUE         = {0.53f, 0.81f, 0.92f, 1.0f}; // スカイブルー

    ///////////////////////////////////////////
    // パステル系 / Pastel Colors
    ///////////////////////////////////////////
    constexpr COLOR PASTEL_PINK      = {1.0f, 0.8f, 0.86f, 1.0f}; // パステルピンク
    constexpr COLOR PASTEL_YELLOW    = {1.0f, 1.0f, 0.6f, 1.0f};  // パステルイエロー
    constexpr COLOR PASTEL_GREEN     = {0.6f, 1.0f, 0.6f, 1.0f};  // パステルグリーン
    constexpr COLOR PASTEL_BLUE      = {0.6f, 0.8f, 1.0f, 1.0f};  // パステルブルー

    ///////////////////////////////////////////
    // ビビッド系 / Vibrant Colors
    ///////////////////////////////////////////
    constexpr COLOR NEON_GREEN       = {0.22f, 1.0f, 0.078f, 1.0f}; // ネオングリーン
    constexpr COLOR HOT_PINK         = {1.0f, 0.41f, 0.71f, 1.0f}; // ホットピンク
    constexpr COLOR ELECTRIC_BLUE    = {0.49f, 0.98f, 1.0f, 1.0f}; // エレクトリックブルー

    ///////////////////////////////////////////
    // ライト系 / Light Colors
    ///////////////////////////////////////////
    constexpr COLOR LIGHT_RED        = {1.0f, 0.68f, 0.68f, 1.0f}; // ライトレッド
    constexpr COLOR LIGHT_GREEN      = {0.68f, 1.0f, 0.68f, 1.0f}; // ライトグリーン
    constexpr COLOR LIGHT_BLUE       = {0.68f, 0.85f, 0.9f, 1.0f}; // ライトブルー
    constexpr COLOR LIGHT_YELLOW     = {1.0f, 1.0f, 0.68f, 1.0f}; // ライトイエロー
}

#endif // COLOR_ALIASES_H
