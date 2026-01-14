/****************************************
 * @file enemy_bullet.h
 * @brief 敵の弾
 * @author
 * @date 2026/01/10
 ****************************************/

#ifndef ENEMY_BULLET_H
#define ENEMY_BULLET_H

#include <DirectXMath.h>
#include "collision.h"

 //======================================
 // 調整用定数
 //======================================
namespace EnemyBulletConfig
{
    constexpr float SPEED = 20.0f;           // 弾速（見てから回避可能）
    constexpr float RADIUS = 0.3f;           // 弾の半径
    constexpr float MAX_LIFETIME = 10.0f;    // 最大生存時間（秒）
    constexpr int   DAMAGE = 1;              // ダメージ量
    constexpr float PLAYER_HIT_RADIUS = 1.0f; // プレイヤー当たり判定半径
}

//======================================
// グローバル管理関数
//======================================
void EnemyBullet_Initialize();
void EnemyBullet_Finalize();
void EnemyBullet_Update(float dt);
void EnemyBullet_Draw();
void EnemyBullet_DrawDebug();

void EnemyBullet_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& direction);
void EnemyBullet_Clear();

int EnemyBullet_GetCount();
const DirectX::XMFLOAT3& EnemyBullet_GetPosition(int index);
AABB EnemyBullet_GetAABB(int index);

#endif // ENEMY_BULLET_H