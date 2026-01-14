/****************************************
 * @file enemy_bullet.cpp
 * @brief 敵の弾の実装
 * @author Natsume Shidara
 * @date 2026/01/10
 ****************************************/

#include "enemy_bullet.h"
#include "player.h"
#include "map.h"
#include "model.h"
#include "debug_renderer.h"
#include "trail.h"

#include <vector>
#include <algorithm>

using namespace DirectX;

//======================================
// 弾クラス（内部用）
//======================================
class EnemyBulletInternal
{
private:
    XMFLOAT3 m_Position;
    XMFLOAT3 m_Direction;
    float m_LifeTime;
    bool m_Active;

public:
    EnemyBulletInternal(const XMFLOAT3& position, const XMFLOAT3& direction)
        : m_Position(position)
        , m_LifeTime(0.0f)
        , m_Active(true)
    {
        // 方向を正規化
        XMStoreFloat3(&m_Direction, XMVector3Normalize(XMLoadFloat3(&direction)));
    }

    void Update(float dt)
    {
        if (!m_Active) return;

        // 移動
        XMVECTOR pos = XMLoadFloat3(&m_Position);
        XMVECTOR dir = XMLoadFloat3(&m_Direction);
        pos += dir * EnemyBulletConfig::SPEED * dt;
        XMStoreFloat3(&m_Position, pos);

        // トレイル生成（オレンジ色）
        Trail_Create(m_Position, { 1.0f, 0.4f, 0.1f, 0.3f }, 1.5f, 0.8f);

        // 寿命チェック
        m_LifeTime += dt;
        if (m_LifeTime >= EnemyBulletConfig::MAX_LIFETIME)
        {
            m_Active = false;
            return;
        }

        // プレイヤーとの当たり判定
        XMFLOAT3 playerPos = Player_GetPosition();
        playerPos.y += 1.0f;  // プレイヤーの中心

        XMVECTOR toPlayer = XMLoadFloat3(&playerPos) - pos;
        float distance = XMVectorGetX(XMVector3Length(toPlayer));

        float hitRadius = EnemyBulletConfig::RADIUS + EnemyBulletConfig::PLAYER_HIT_RADIUS;
        if (distance < hitRadius)
        {
            // ヒット
            Player_TakeDamage(EnemyBulletConfig::DAMAGE);
            m_Active = false;
            return;
        }

        // マップとの当たり判定（壁に当たったら消える）
        int mapCount = Map_GetObjectCount();
        for (int i = 0; i < mapCount; i++)
        {
            AABB aabb = Map_GetObject(i)->Aabb;

            // 簡易判定（点がAABB内）
            if (m_Position.x >= aabb.min.x && m_Position.x <= aabb.max.x &&
                m_Position.y >= aabb.min.y && m_Position.y <= aabb.max.y &&
                m_Position.z >= aabb.min.z && m_Position.z <= aabb.max.z)
            {
                m_Active = false;
                return;
            }
        }
    }

    const XMFLOAT3& GetPosition() const { return m_Position; }
    const XMFLOAT3& GetDirection() const { return m_Direction; }
    bool IsActive() const { return m_Active; }

    AABB GetAABB() const
    {
        AABB aabb;
        float r = EnemyBulletConfig::RADIUS;
        aabb.min = { m_Position.x - r, m_Position.y - r, m_Position.z - r };
        aabb.max = { m_Position.x + r, m_Position.y + r, m_Position.z + r };
        return aabb;
    }
};

//======================================
// 管理用変数
//======================================
static std::vector<EnemyBulletInternal*> g_Bullets;
static MODEL* g_pBulletModel = nullptr;

//======================================
// グローバル管理関数
//======================================

void EnemyBullet_Initialize()
{
    g_Bullets.clear();
    g_Bullets.reserve(256);

    // TODO: 弾モデル読み込み
    // g_pBulletModel = ModelLoad("assets/fbx/enemy_bullet.fbx", 1.0f);
}

void EnemyBullet_Finalize()
{
    for (auto* pBullet : g_Bullets)
    {
        delete pBullet;
    }
    g_Bullets.clear();

    if (g_pBulletModel)
    {
        ModelRelease(g_pBulletModel);
        g_pBulletModel = nullptr;
    }
}

void EnemyBullet_Update(float dt)
{
    // 全弾を更新
    for (auto* pBullet : g_Bullets)
    {
        pBullet->Update(dt);
    }

    // 非アクティブな弾を削除
    for (int i = static_cast<int>(g_Bullets.size()) - 1; i >= 0; i--)
    {
        if (!g_Bullets[i]->IsActive())
        {
            delete g_Bullets[i];
            g_Bullets.erase(g_Bullets.begin() + i);
        }
    }
}

void EnemyBullet_Draw()
{
    for (const auto* pBullet : g_Bullets)
    {
        if (!pBullet->IsActive()) continue;

        XMFLOAT3 pos = pBullet->GetPosition();
        XMMATRIX world = XMMatrixTranslation(pos.x, pos.y, pos.z);

        if (g_pBulletModel)
        {
            ModelDraw(g_pBulletModel, world);
        }
        else
        {
            // モデルがない場合はデバッグ描画で代用
            Sphere sphere;
            sphere.center = pos;
            sphere.radius = EnemyBulletConfig::RADIUS;
            DebugRenderer::DrawSphere(sphere, { 1.0f, 0.3f, 0.0f, 1.0f });
        }
    }
}

void EnemyBullet_DrawDebug()
{
    for (const auto* pBullet : g_Bullets)
    {
        if (!pBullet->IsActive()) continue;

        Sphere sphere;
        sphere.center = pBullet->GetPosition();
        sphere.radius = EnemyBulletConfig::RADIUS;
        DebugRenderer::DrawSphere(sphere, { 1.0f, 0.3f, 0.0f, 1.0f });

        // 進行方向
        XMFLOAT3 start = pBullet->GetPosition();
        XMFLOAT3 end;
        XMStoreFloat3(&end, XMLoadFloat3(&start) + XMLoadFloat3(&pBullet->GetDirection()) * 1.0f);
        DebugRenderer::DrawLine(start, end, { 1.0f, 1.0f, 0.0f, 1.0f });
    }
}

void EnemyBullet_Create(const XMFLOAT3& position, const XMFLOAT3& direction)
{
    g_Bullets.push_back(new EnemyBulletInternal(position, direction));
}

void EnemyBullet_Clear()
{
    for (auto* pBullet : g_Bullets)
    {
        delete pBullet;
    }
    g_Bullets.clear();
}

int EnemyBullet_GetCount()
{
    return static_cast<int>(g_Bullets.size());
}

const XMFLOAT3& EnemyBullet_GetPosition(int index)
{
    static XMFLOAT3 zero = { 0, 0, 0 };
    if (index < 0 || index >= static_cast<int>(g_Bullets.size()))
        return zero;
    return g_Bullets[index]->GetPosition();
}

AABB EnemyBullet_GetAABB(int index)
{
    if (index < 0 || index >= static_cast<int>(g_Bullets.size()))
        return AABB{};
    return g_Bullets[index]->GetAABB();
}