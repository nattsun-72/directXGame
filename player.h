/****************************************
 * @file player.h
 * @brief プレイヤー制御（ステートマシン対応版）
 * @author Natsume Shidara
 * @date 2025/10/31
 * @update 2026/01/10 - リファクタリング
 ****************************************/

#ifndef PLAYER_H
#define PLAYER_H

#include <DirectXMath.h>
#include "collision.h"

 //--------------------------------------
 // プレイヤー状態列挙型
 //--------------------------------------
enum class PlayerState
{
    // 地上状態
    Idle,           // 待機
    Walk,           // 歩行
    Attack,         // ブレード攻撃（地上）
    Step,           // ステップ回避（地上・空中共通）

    // 空中状態
    Jump,           // ジャンプ / 二段ジャンプ
    Fall,           // 落下
    AirDashCharge,  // 空中ダッシュ溜め
    AirDash,        // 空中ダッシュ攻撃

    // 状態数（内部用）
    Count
};

//--------------------------------------
// 基本関数
//--------------------------------------
void Player_Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front);
void Player_Finalize();
void Player_Update(double elapsed_time);
void Player_Draw();
void Player_DrawShadow();
void Player_DrawDebug();

//--------------------------------------
// ゲッター
//--------------------------------------
const DirectX::XMFLOAT3& Player_GetPosition();
const DirectX::XMFLOAT3& Player_GetFront();
const DirectX::XMFLOAT3& Player_GetVelocity();
PlayerState Player_GetState();
int Player_GetHP();
int Player_GetMaxHP();
bool Player_IsGrounded();
int Player_GetJumpCount();
int Player_GetAirDashCount();
OBB Player_GetWorldOBB(const DirectX::XMVECTOR& position);

//--------------------------------------
// セッター
//--------------------------------------
void Player_SetPosition(const DirectX::XMFLOAT3& position);

//--------------------------------------
// 外部操作
//--------------------------------------
void Player_TakeDamage(int damage);
void Player_RecoverAirDash(int amount = 1);

#endif // PLAYER_H