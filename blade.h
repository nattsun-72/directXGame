/****************************************
 * @file blade.h
 * @brief ブレード制御（一人称視点）
 * @author Natsume Shidara
 * @date 2026/01/07
 * @update 2026/01/10 - リファクタリング
 ****************************************/

#ifndef BLADE_H
#define BLADE_H

#include <DirectXMath.h>

 //======================================
 // ブレードの状態
 //======================================
enum class BladeState
{
    Idle,              // 待機
    FreeSlice,         // 自由斬撃（右クリック+ドラッグ）
    FixedAttack,       // 固定攻撃（左クリック）- 縦斬り
    HorizontalSlash,   // 横なぎ攻撃（AirDash連動）
};

//======================================
// 初期化・終了
//======================================
void Blade_Initialize();
void Blade_Finalize();

//======================================
// 更新・描画
//======================================
void Blade_Update(double elapsed_time);
void Blade_Draw();
void Blade_DrawShadow();
void Blade_DebugDraw();

//======================================
// 外部トリガー（プレイヤーから呼び出し）
//======================================
void Blade_TriggerVerticalSlash();    // 縦斬り（通常攻撃）
void Blade_TriggerHorizontalSlash();  // 横なぎ（AirDash攻撃）

//======================================
// ゲッター
//======================================
BladeState Blade_GetState();
bool Blade_IsAttacking();
int Blade_GetDebugMode();

//======================================
// 設定
//======================================
void Blade_SetScreenOffset(const DirectX::XMFLOAT3& offset);
void Blade_SetScale(float scale);

#endif // BLADE_H