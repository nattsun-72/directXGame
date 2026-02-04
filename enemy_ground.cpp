/****************************************
 * @file enemy_ground.cpp
 * @brief 地上型敵の実装（PhysicsModelコンポジション版）
 * @author Natsume Shidara
 * @date 2026/01/10
 * @update 2026/01/13 - サウンド対応
 ****************************************/

#include "enemy_ground.h"
#include "enemy_bullet.h"
#include "player.h"
#include "map.h"
#include "debug_renderer.h"
#include "texture.h"
#include "collider_generator.h"
#include "sound_manager.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

//======================================
// 共有リソース
//======================================
static MODEL* g_pEnemyGroundModel = nullptr;

void EnemyGround_InitializeShared()
{
    using namespace EnemyGroundConfig;

    if (!g_pEnemyGroundModel)
    {
        g_pEnemyGroundModel = ModelLoad(MODEL_PATH, MODEL_SCALE, false, TEXTURE_PATH);
    }
}

void EnemyGround_FinalizeShared()
{
    if (g_pEnemyGroundModel)
    {
        ModelRelease(g_pEnemyGroundModel);
        g_pEnemyGroundModel = nullptr;
    }
}

//======================================
// ヘルパー関数
//======================================
namespace
{
    XMFLOAT4 GetStateColor(const Enemy::State* pState)
    {
        if (dynamic_cast<const EnemyGround::StateIdle*>(pState))
            return { 0.5f, 0.5f, 0.5f, 1.0f };
        if (dynamic_cast<const EnemyGround::StateApproach*>(pState))
            return { 0.0f, 1.0f, 0.0f, 1.0f };
        if (dynamic_cast<const EnemyGround::StateAttack*>(pState))
            return { 1.0f, 0.0f, 0.0f, 1.0f };
        if (dynamic_cast<const EnemyGround::StateReload*>(pState))
            return { 1.0f, 1.0f, 0.0f, 1.0f };
        if (dynamic_cast<const EnemyGround::StateRetreat*>(pState))
            return { 0.0f, 0.5f, 1.0f, 1.0f };

        return { 1.0f, 1.0f, 1.0f, 1.0f };
    }

    void DrawEnemyGround(const PhysicsModel* pPhysics)
    {
        if (pPhysics)
        {
            MODEL* pModel = const_cast<PhysicsModel*>(pPhysics)->GetModel();
            if (pModel)
            {
                XMFLOAT4X4 matWorld = pPhysics->GetRigidBody()->GetWorldMatrix(pPhysics->GetScale());
                ModelDraw(pModel, XMLoadFloat4x4(&matWorld));
            }
        }
    }

#ifdef _DEBUG
    void DrawEnemyGroundDebug(const PhysicsModel* pPhysics, const XMFLOAT4& color)
    {
        if (!pPhysics) return;

        const RigidBody* rb = pPhysics->GetRigidBody();
        Collider col = rb->GetWorldCollider();

        switch (col.type)
        {
        case ColliderType::Box:
            DebugRenderer::DrawOBB(col.obb, color);
            break;
        case ColliderType::Sphere:
            DebugRenderer::DrawSphere(col.sphere, color);
            break;
        }

        XMFLOAT3 pos = rb->GetPosition();
        XMFLOAT3 lineEnd;
        XMFLOAT4 rot = rb->GetRotation();
        XMVECTOR quat = XMLoadFloat4(&rot);
        XMVECTOR forward = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), quat);
        XMStoreFloat3(&lineEnd, XMLoadFloat3(&pos) + forward * 2.0f);

        DebugRenderer::DrawLine(pos, lineEnd, { 1.0f, 0.0f, 1.0f, 1.0f });
    }
#endif

    float CalculateColliderVolume(const Collider& collider)
    {
        switch (collider.type)
        {
        case ColliderType::Box:
        {
            const auto& extents = collider.obb.extents;
            return (extents.x * 2.0f) * (extents.y * 2.0f) * (extents.z * 2.0f);
        }
        case ColliderType::Sphere:
        {
            constexpr float SPHERE_VOLUME_CONSTANT = 4.18879f;
            return SPHERE_VOLUME_CONSTANT * std::pow(collider.sphere.radius, 3.0f);
        }
        default:
            return 1.0f;
        }
    }
}

//======================================
// EnemyGround 実装
//======================================

EnemyGround::EnemyGround(const XMFLOAT3& position)
{
    using namespace EnemyGroundConfig;

    if (g_pEnemyGroundModel)
    {
        m_pPhysics = new PhysicsModel(
            g_pEnemyGroundModel,
            position,
            XMFLOAT3(0.0f, 0.0f, 0.0f),
            MASS,
            ColliderType::Box
        );

        const Collider& col = m_pPhysics->GetRigidBody()->GetWorldCollider();
        float volume = CalculateColliderVolume(col);
        m_pPhysics->SetRootVolume(volume);
    }

    m_Position = position;
    m_Front = { 0.0f, 0.0f, 1.0f };
    m_VolumeRatio = 1.0f;
    m_IsDestroyed = false;

    m_pState = new StateIdle(this);
}

EnemyGround::~EnemyGround()
{
    if (m_pPhysics)
    {
        delete m_pPhysics;
        m_pPhysics = nullptr;
    }
}

bool EnemyGround::IsDestroy() const
{
    if (m_pPhysics && m_pPhysics->IsDead())
        return true;

    return m_IsDestroyed;
}

XMFLOAT3 EnemyGround::GetPosition() const
{
    if (m_pPhysics)
    {
        return m_pPhysics->GetPosition();
    }
    return m_Position;
}

void EnemyGround::SetPosition(const XMFLOAT3& pos)
{
    if (m_pPhysics)
    {
        m_pPhysics->SetPosition(pos);
    }
    m_Position = pos;
}

void EnemyGround::UpdateDistanceAndFacing()
{
    XMFLOAT3 myPos = GetPosition();
    XMFLOAT3 playerPos = Player_GetPosition();
    XMVECTOR toPlayer = XMLoadFloat3(&playerPos) - XMLoadFloat3(&myPos);

    m_DistanceToPlayer = XMVectorGetX(XMVector3Length(toPlayer));

    toPlayer = XMVectorSetY(toPlayer, 0.0f);
    if (XMVectorGetX(XMVector3LengthSq(toPlayer)) > 0.01f)
    {
        XMStoreFloat3(&m_Front, XMVector3Normalize(toPlayer));
    }
}

void EnemyGround::FireBullet()
{
    XMFLOAT3 myPos = GetPosition();

    XMFLOAT3 firePos = myPos;
    firePos.y += 0.5f;

    XMVECTOR vFirePos = XMLoadFloat3(&firePos) + XMLoadFloat3(&m_Front) * 1.0f;
    XMStoreFloat3(&firePos, vFirePos);

    XMFLOAT3 playerPos = Player_GetPosition();
    playerPos.y += 1.0f;

    XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&playerPos) - XMLoadFloat3(&firePos));
    XMFLOAT3 fireDir;
    XMStoreFloat3(&fireDir, dir);

    EnemyBullet_Create(firePos, fireDir);

    // 発射SE
    SoundManager_PlaySE(SOUND_SE_ENEMY_FIRE);
}

XMFLOAT3 EnemyGround::CalculateRetreatDirection()
{
    XMVECTOR retreatDir = XMVectorNegate(XMLoadFloat3(&m_Front));
    retreatDir = XMVectorSetY(retreatDir, 0.0f);
    retreatDir = XMVector3Normalize(retreatDir);

    XMFLOAT3 result;
    XMStoreFloat3(&result, retreatDir);

    if (CheckWall(result, EnemyGroundConfig::WALL_CHECK_DISTANCE))
    {
        XMVECTOR up = XMVectorSet(0, 1, 0, 0);
        XMVECTOR left = XMVector3Normalize(XMVector3Cross(up, retreatDir));
        XMVECTOR right = XMVectorNegate(left);

        XMFLOAT3 leftDir, rightDir;
        XMStoreFloat3(&leftDir, left);
        XMStoreFloat3(&rightDir, right);

        if (!CheckWall(leftDir, EnemyGroundConfig::WALL_CHECK_DISTANCE))
        {
            return leftDir;
        }
        if (!CheckWall(rightDir, EnemyGroundConfig::WALL_CHECK_DISTANCE))
        {
            return rightDir;
        }
        return { 0.0f, 0.0f, 0.0f };
    }

    return result;
}

bool EnemyGround::CheckWall(const XMFLOAT3& direction, float distance)
{
    XMFLOAT3 myPos = GetPosition();
    XMVECTOR checkPos = XMLoadFloat3(&myPos) + XMLoadFloat3(&direction) * distance;
    XMFLOAT3 check;
    XMStoreFloat3(&check, checkPos);

    int count = Map_GetObjectCount();
    for (int i = 0; i < count; i++)
    {
        AABB aabb = Map_GetObject(i)->Aabb;

        if (check.x >= aabb.min.x && check.x <= aabb.max.x &&
            check.z >= aabb.min.z && check.z <= aabb.max.z)
        {
            return true;
        }
    }

    return false;
}

void EnemyGround::Move(const XMFLOAT3& direction, float speed)
{
    if (!m_pPhysics) return;

    XMFLOAT3 currentVel = m_pPhysics->GetVelocity();
    XMVECTOR vDir = XMLoadFloat3(&direction);
    vDir = XMVectorSetY(vDir, 0.0f);

    if (XMVectorGetX(XMVector3LengthSq(vDir)) > 0.01f)
    {
        vDir = XMVector3Normalize(vDir);
    }

    XMFLOAT3 newVel;
    XMStoreFloat3(&newVel, vDir * speed);
    newVel.y = currentVel.y;

    m_pPhysics->SetVelocity(newVel);
}

void EnemyGround::StopMovement()
{
    if (!m_pPhysics) return;

    XMFLOAT3 vel = m_pPhysics->GetVelocity();
    vel.x = 0.0f;
    vel.z = 0.0f;
    m_pPhysics->SetVelocity(vel);
}

void EnemyGround::CheckVolumeAndDeath()
{
    using namespace EnemyGroundConfig;

    if (!m_pPhysics) return;

    const Collider& col = m_pPhysics->GetRigidBody()->GetWorldCollider();
    float currentVolume = CalculateColliderVolume(col);
    float rootVolume = m_pPhysics->GetRootVolume();

    if (rootVolume > 0.0f)
    {
        float ratio = currentVolume / rootVolume;
        m_VolumeRatio = ratio;

        if (ratio <= DEATH_VOLUME_THRESHOLD)
        {
            if (!m_IsDestroyed)
            {
                // 撃破SE（初回のみ）
                SoundManager_PlaySE(SOUND_SE_ENEMY_DEATH);
            }
            m_IsDestroyed = true;
        }
    }
}

//======================================
// StateIdle 実装
//======================================

void EnemyGround::StateIdle::Update(float dt)
{
    (void)dt;

    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);

    if (pOwner->m_pPhysics)
    {
        pOwner->m_pPhysics->Update(dt);
    }

    pOwner->UpdateDistanceAndFacing();

    float dist = pOwner->GetDistanceToPlayer();

    if (dist < EnemyGroundConfig::RANGE_NEAR)
    {
        pOwner->ChangeState(new StateRetreat(pOwner));
    }
    else if (dist > EnemyGroundConfig::RANGE_FAR)
    {
        pOwner->ChangeState(new StateApproach(pOwner));
    }
    else
    {
        pOwner->ChangeState(new StateAttack(pOwner));
    }
}

void EnemyGround::StateIdle::Draw() const
{
    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);
    DrawEnemyGround(pOwner->m_pPhysics);
}

void EnemyGround::StateIdle::DrawDebug() const
{
#ifdef _DEBUG
    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);
    DrawEnemyGroundDebug(pOwner->m_pPhysics, GetStateColor(this));
#endif
}

//======================================
// StateApproach 実装
//======================================

void EnemyGround::StateApproach::Update(float dt)
{
    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);

    if (pOwner->m_pPhysics)
    {
        pOwner->m_pPhysics->Update(dt);
    }

    pOwner->UpdateDistanceAndFacing();

    float dist = pOwner->GetDistanceToPlayer();

    if (dist < EnemyGroundConfig::RANGE_NEAR)
    {
        pOwner->StopMovement();
        pOwner->ChangeState(new StateRetreat(pOwner));
        return;
    }
    if (dist <= EnemyGroundConfig::RANGE_FAR)
    {
        pOwner->StopMovement();
        pOwner->ChangeState(new StateAttack(pOwner));
        return;
    }

    pOwner->Move(pOwner->m_Front, EnemyGroundConfig::APPROACH_SPEED);
}

void EnemyGround::StateApproach::Draw() const
{
    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);
    DrawEnemyGround(pOwner->m_pPhysics);
}

void EnemyGround::StateApproach::DrawDebug() const
{
#ifdef _DEBUG
    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);
    DrawEnemyGroundDebug(pOwner->m_pPhysics, GetStateColor(this));
#endif
}

//======================================
// StateAttack 実装
//======================================

void EnemyGround::StateAttack::Enter()
{
    m_StateTimer = 0.0f;
    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);
    pOwner->GetFireTimer() = 0.0f;
    pOwner->StopMovement();
}

void EnemyGround::StateAttack::Update(float dt)
{
    m_StateTimer += dt;

    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);

    if (pOwner->m_pPhysics)
    {
        pOwner->m_pPhysics->Update(dt);
    }

    pOwner->UpdateDistanceAndFacing();

    if (m_StateTimer >= EnemyGroundConfig::ATTACK_DURATION)
    {
        pOwner->ChangeState(new StateReload(pOwner));
        return;
    }

    float dist = pOwner->GetDistanceToPlayer();

    if (dist < EnemyGroundConfig::RANGE_NEAR)
    {
        pOwner->ChangeState(new StateRetreat(pOwner));
        return;
    }
    if (dist > EnemyGroundConfig::RANGE_FAR)
    {
        pOwner->ChangeState(new StateApproach(pOwner));
        return;
    }

    pOwner->GetFireTimer() += dt;
    if (pOwner->GetFireTimer() >= EnemyGroundConfig::FIRE_INTERVAL)
    {
        //pOwner->FireBullet();
        pOwner->GetFireTimer() = 0.0f;
    }
}

void EnemyGround::StateAttack::Draw() const
{
    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);
    DrawEnemyGround(pOwner->m_pPhysics);
}

void EnemyGround::StateAttack::DrawDebug() const
{
#ifdef _DEBUG
    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);
    DrawEnemyGroundDebug(pOwner->m_pPhysics, GetStateColor(this));
#endif
}

//======================================
// StateReload 実装
//======================================

void EnemyGround::StateReload::Update(float dt)
{
    m_StateTimer += dt;

    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);

    if (pOwner->m_pPhysics)
    {
        pOwner->m_pPhysics->Update(dt);
    }

    pOwner->UpdateDistanceAndFacing();

    if (m_StateTimer >= EnemyGroundConfig::RELOAD_DURATION)
    {
        float dist = pOwner->GetDistanceToPlayer();

        if (dist < EnemyGroundConfig::RANGE_NEAR)
        {
            pOwner->ChangeState(new StateRetreat(pOwner));
        }
        else if (dist > EnemyGroundConfig::RANGE_FAR)
        {
            pOwner->ChangeState(new StateApproach(pOwner));
        }
        else
        {
            pOwner->ChangeState(new StateAttack(pOwner));
        }
    }
}

void EnemyGround::StateReload::Draw() const
{
    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);
    DrawEnemyGround(pOwner->m_pPhysics);
}

void EnemyGround::StateReload::DrawDebug() const
{
#ifdef _DEBUG
    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);
    DrawEnemyGroundDebug(pOwner->m_pPhysics, GetStateColor(this));
#endif
}

//======================================
// StateRetreat 実装
//======================================

void EnemyGround::StateRetreat::Update(float dt)
{
    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);

    if (pOwner->m_pPhysics)
    {
        pOwner->m_pPhysics->Update(dt);
    }

    pOwner->UpdateDistanceAndFacing();

    float dist = pOwner->GetDistanceToPlayer();

    if (dist >= EnemyGroundConfig::RANGE_NEAR)
    {
        pOwner->StopMovement();
        pOwner->ChangeState(new StateAttack(pOwner));
        return;
    }

    XMFLOAT3 retreatDir = pOwner->CalculateRetreatDirection();
    pOwner->Move(retreatDir, EnemyGroundConfig::RETREAT_SPEED);
}

void EnemyGround::StateRetreat::Draw() const
{
    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);
    DrawEnemyGround(pOwner->m_pPhysics);
}

void EnemyGround::StateRetreat::DrawDebug() const
{
#ifdef _DEBUG
    EnemyGround* pOwner = static_cast<EnemyGround*>(m_pOwner);
    DrawEnemyGroundDebug(pOwner->m_pPhysics, GetStateColor(this));
#endif
}

//======================================
// 切断結果からEnemyGroundを生成
//======================================
EnemyGround* EnemyGround_CreateFromSlice(const SlicedEnemyParams& params)
{
    using namespace EnemyGroundConfig;

    if (params.meshes.empty() || !params.originalModel)
    {
        return nullptr;
    }

    MODEL* newModel = ModelCreateFromData(params.meshes, params.originalModel);
    if (!newModel)
    {
        return nullptr;
    }

    EnemyGround* pEnemy = new EnemyGround(params.position);

    if (pEnemy->m_pPhysics)
    {
        delete pEnemy->m_pPhysics;
    }

    XMVECTOR vVel = XMLoadFloat3(&params.velocity);
    XMVECTOR vNormal = XMLoadFloat3(&params.planeNormal);
    float sign = params.isFrontSide ? 1.0f : -1.0f;
    vVel = vVel + vNormal * (SLICE_SEPARATION_SPEED * sign);

    XMFLOAT3 newVelocity;
    XMStoreFloat3(&newVelocity, vVel);

    XMVECTOR vPos = XMLoadFloat3(&params.position);
    vPos = vPos + vNormal * (SLICE_OFFSET_DISTANCE * sign);
    XMFLOAT3 newPosition;
    XMStoreFloat3(&newPosition, vPos);

    pEnemy->m_pPhysics = new PhysicsModel(
        newModel,
        newPosition,
        newVelocity,
        params.mass,
        params.colliderType
    );

    pEnemy->m_pPhysics->SetRootVolume(params.rootVolume);

    pEnemy->m_pPhysics->SetIgnoreCollisionTimer(SLICE_IMMUNITY_TIME);

    XMFLOAT3 torque;
    XMStoreFloat3(&torque, vNormal * (SLICE_TORQUE_STRENGTH * sign));
    pEnemy->m_pPhysics->GetRigidBody()->AddTorque(torque);

    pEnemy->m_Position = newPosition;

    ModelRelease(newModel);

    // 切断SE
    SoundManager_PlaySE(SOUND_SE_ENEMY_SLICE);

    return pEnemy;
}