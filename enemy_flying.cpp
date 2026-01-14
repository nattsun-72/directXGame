/****************************************
 * @file enemy_flying.cpp
 * @brief 飛行型敵の実装
 * @author NatsemeShidara
 * @date 2026/01/13
 * @update 2026/01/13 - サウンド対応・デバッグ分離
 ****************************************/

#include "enemy_flying.h"
#include "player.h"
#include "stage.h"
#include "texture.h"
#include "collider_generator.h"
#include "trail.h"
#include "rigid_body.h"
#include "sound_manager.h"

#ifdef _DEBUG
#include "debug_renderer.h"
#endif

#include <algorithm>
#include <cmath>

using namespace DirectX;

//======================================
// 定数（地形衝突用）
//======================================
namespace
{
    constexpr float TERRAIN_MIN_HEIGHT = 1.0f;
}

//======================================
// 共有リソース
//======================================
static MODEL* g_pEnemyFlyingModel = nullptr;

void EnemyFlying_InitializeShared()
{
    using namespace EnemyFlyingConfig;

    if (!g_pEnemyFlyingModel)
    {
        g_pEnemyFlyingModel = ModelLoad(MODEL_PATH, MODEL_SCALE, true, TEXTURE_PATH);
    }
}

void EnemyFlying_FinalizeShared()
{
    if (g_pEnemyFlyingModel)
    {
        ModelRelease(g_pEnemyFlyingModel);
        g_pEnemyFlyingModel = nullptr;
    }
}

//======================================
// ヘルパー関数
//======================================
namespace
{
#ifdef _DEBUG
    XMFLOAT4 GetFlyingStateColor(const Enemy::State* pState)
    {
        if (dynamic_cast<const EnemyFlying::StateHover*>(pState))
            return { 0.3f, 0.8f, 1.0f, 1.0f };
        if (dynamic_cast<const EnemyFlying::StateWindup*>(pState))
            return { 1.0f, 0.8f, 0.0f, 1.0f };
        if (dynamic_cast<const EnemyFlying::StateCharge*>(pState))
            return { 1.0f, 0.0f, 0.0f, 1.0f };
        if (dynamic_cast<const EnemyFlying::StateRetreat*>(pState))
            return { 0.5f, 0.0f, 1.0f, 1.0f };
        if (dynamic_cast<const EnemyFlying::StateCooldown*>(pState))
            return { 0.5f, 0.5f, 0.5f, 1.0f };

        return { 1.0f, 1.0f, 1.0f, 1.0f };
    }
#endif

    void DrawEnemyFlying(const PhysicsModel* pPhysics)
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

    void DrawEnemyFlyingShadow(const PhysicsModel* pPhysics)
    {
        if (pPhysics)
        {
            MODEL* pModel = const_cast<PhysicsModel*>(pPhysics)->GetModel();
            if (pModel)
            {
                XMFLOAT4X4 matWorld = pPhysics->GetRigidBody()->GetWorldMatrix(pPhysics->GetScale());
                ModelDrawShadow(pModel, XMLoadFloat4x4(&matWorld));
            }
        }
    }

#ifdef _DEBUG
    void DrawEnemyFlyingDebug(const PhysicsModel* pPhysics, const XMFLOAT4& color)
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
        XMFLOAT4 rot = rb->GetRotation();
        XMVECTOR quat = XMLoadFloat4(&rot);
        XMVECTOR forward = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), quat);

        XMFLOAT3 lineEnd;
        XMStoreFloat3(&lineEnd, XMLoadFloat3(&pos) + forward * 3.0f);
        DebugRenderer::DrawLine(pos, lineEnd, { 1.0f, 0.5f, 0.0f, 1.0f });
    }
#endif

    float CalculateFlyingColliderVolume(const Collider& collider)
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
// EnemyFlying 実装
//======================================

EnemyFlying::EnemyFlying(const XMFLOAT3& position)
{
    using namespace EnemyFlyingConfig;

    if (g_pEnemyFlyingModel)
    {
        m_pPhysics = new PhysicsModel(
            g_pEnemyFlyingModel,
            position,
            XMFLOAT3(0.0f, 0.0f, 0.0f),
            MASS,
            ColliderType::Sphere
        );

        m_pPhysics->GetRigidBody()->SetGravityEnabled(false);

        RigidBody::Params params = m_pPhysics->GetRigidBody()->GetParams();
        params.linearDrag = DRAG;
        m_pPhysics->GetRigidBody()->SetParams(params);

        const Collider& col = m_pPhysics->GetRigidBody()->GetWorldCollider();
        float volume = CalculateFlyingColliderVolume(col);
        m_pPhysics->SetRootVolume(volume);
    }

    m_Position = position;
    m_Front = { 0.0f, 0.0f, 1.0f };
    m_VolumeRatio = 1.0f;
    m_IsDestroyed = false;

    m_pState = new StateHover(this);

    m_HoverTimer = 0.0f;
    m_CircleAngle = static_cast<float>(rand() % 360) * (3.14159f / 180.0f);
    m_ChargeTarget = { 0.0f, 0.0f, 0.0f };
    m_ChargeDirection = { 0.0f, 0.0f, 1.0f };
}

EnemyFlying::~EnemyFlying()
{
    if (m_pPhysics)
    {
        delete m_pPhysics;
        m_pPhysics = nullptr;
    }
}

bool EnemyFlying::IsDestroy() const
{
    if (m_pPhysics && m_pPhysics->IsDead())
        return true;

    return m_IsDestroyed;
}

XMFLOAT3 EnemyFlying::GetPosition() const
{
    if (m_pPhysics)
    {
        return m_pPhysics->GetPosition();
    }
    return m_Position;
}

void EnemyFlying::SetPosition(const XMFLOAT3& pos)
{
    if (m_pPhysics)
    {
        m_pPhysics->SetPosition(pos);
    }
    m_Position = pos;
}

XMFLOAT3 EnemyFlying::GetVelocity() const
{
    if (m_pPhysics)
    {
        return m_pPhysics->GetVelocity();
    }
    return { 0.0f, 0.0f, 0.0f };
}

void EnemyFlying::SetVelocity(const XMFLOAT3& vel)
{
    if (m_pPhysics)
    {
        m_pPhysics->SetVelocity(vel);
    }
}

void EnemyFlying::UpdateDistanceAndFacing()
{
    XMFLOAT3 myPos = GetPosition();
    XMFLOAT3 playerPos = Player_GetPosition();
    XMVECTOR toPlayer = XMLoadFloat3(&playerPos) - XMLoadFloat3(&myPos);

    m_DistanceToPlayer = XMVectorGetX(XMVector3Length(toPlayer));

    if (XMVectorGetX(XMVector3LengthSq(toPlayer)) > 0.01f)
    {
        XMStoreFloat3(&m_Front, XMVector3Normalize(toPlayer));
    }
}

void EnemyFlying::ClampToTerrain()
{
    if (!m_pPhysics) return;

    XMFLOAT3 pos = GetPosition();
    float terrainHeight = Stage_GetTerrainHeight(pos.x, pos.z);
    float minHeight = terrainHeight + TERRAIN_MIN_HEIGHT;

    if (pos.y < minHeight)
    {
        pos.y = minHeight;
        SetPosition(pos);

        XMFLOAT3 vel = GetVelocity();
        if (vel.y < 0.0f)
        {
            vel.y = 0.0f;
            SetVelocity(vel);
        }
    }
}

void EnemyFlying::ApplyHoverForce(float dt)
{
    using namespace EnemyFlyingConfig;

    if (!m_pPhysics) return;

    m_HoverTimer += dt * HOVER_SPEED;

    XMFLOAT3 pos = GetPosition();
    XMFLOAT3 playerPos = Player_GetPosition();

    float terrainHeight = Stage_GetTerrainHeight(pos.x, pos.z);
    float baseHeight = std::max(playerPos.y, terrainHeight);

    float targetHeight = baseHeight + HOVER_HEIGHT_MIN +
        (HOVER_HEIGHT_MAX - HOVER_HEIGHT_MIN) * 0.5f;

    targetHeight += sinf(m_HoverTimer) * HOVER_AMPLITUDE;

    float heightDiff = targetHeight - pos.y;
    float verticalForce = heightDiff * 5.0f;

    XMFLOAT3 vel = GetVelocity();
    vel.y += verticalForce * dt;
    vel.y *= 0.95f;

    SetVelocity(vel);
}

void EnemyFlying::MoveToward(const XMFLOAT3& target, float speed, float dt)
{
    if (!m_pPhysics) return;

    XMFLOAT3 pos = GetPosition();
    XMVECTOR toTarget = XMLoadFloat3(&target) - XMLoadFloat3(&pos);
    float dist = XMVectorGetX(XMVector3Length(toTarget));

    if (dist < 0.1f) return;

    XMVECTOR dir = XMVector3Normalize(toTarget);
    XMFLOAT3 moveDir;
    XMStoreFloat3(&moveDir, dir);

    XMFLOAT3 vel = GetVelocity();
    vel.x += moveDir.x * speed * dt;
    vel.y += moveDir.y * speed * dt;
    vel.z += moveDir.z * speed * dt;

    XMVECTOR vVel = XMLoadFloat3(&vel);
    float currentSpeed = XMVectorGetX(XMVector3Length(vVel));
    if (currentSpeed > speed)
    {
        vVel = XMVector3Normalize(vVel) * speed;
        XMStoreFloat3(&vel, vVel);
    }

    SetVelocity(vel);
}

void EnemyFlying::CircleAroundPlayer(float dt)
{
    using namespace EnemyFlyingConfig;

    if (!m_pPhysics) return;

    m_CircleAngle += CIRCLE_SPEED * dt;

    XMFLOAT3 playerPos = Player_GetPosition();

    float targetX = playerPos.x + cosf(m_CircleAngle) * CIRCLE_RADIUS;
    float targetZ = playerPos.z + sinf(m_CircleAngle) * CIRCLE_RADIUS;

    float terrainHeight = Stage_GetTerrainHeight(targetX, targetZ);
    float targetY = std::max(playerPos.y, terrainHeight) + (HOVER_HEIGHT_MIN + HOVER_HEIGHT_MAX) * 0.5f;

    XMFLOAT3 target = { targetX, targetY, targetZ };
    MoveToward(target, HOVER_MOVE_SPEED * 10.0f, dt);
}

void EnemyFlying::PrepareCharge()
{
    m_ChargeTarget = Player_GetPosition();
    m_ChargeTarget.y += 1.0f;

    float terrainHeight = Stage_GetTerrainHeight(m_ChargeTarget.x, m_ChargeTarget.z);
    if (m_ChargeTarget.y < terrainHeight + TERRAIN_MIN_HEIGHT)
    {
        m_ChargeTarget.y = terrainHeight + TERRAIN_MIN_HEIGHT;
    }

    XMFLOAT3 pos = GetPosition();
    XMVECTOR dir = XMLoadFloat3(&m_ChargeTarget) - XMLoadFloat3(&pos);
    dir = XMVector3Normalize(dir);
    XMStoreFloat3(&m_ChargeDirection, dir);
}

void EnemyFlying::ExecuteCharge(float dt)
{
    using namespace EnemyFlyingConfig;

    if (!m_pPhysics) return;

    XMFLOAT3 vel = GetVelocity();
    XMVECTOR vVel = XMLoadFloat3(&vel);
    XMVECTOR vDir = XMLoadFloat3(&m_ChargeDirection);

    vVel = vVel + vDir * CHARGE_ACCELERATION * dt;

    float speed = XMVectorGetX(XMVector3Length(vVel));
    if (speed > CHARGE_SPEED)
    {
        vVel = XMVector3Normalize(vVel) * CHARGE_SPEED;
    }

    XMStoreFloat3(&vel, vVel);
    SetVelocity(vel);

    ClampToTerrain();

    // 軌跡エフェクト
    XMFLOAT3 pos = GetPosition();
    Trail_Create(pos, { 1.0f, 0.3f, 0.1f, 0.8f }, 0.5f, 0.3);
}

bool EnemyFlying::HasReachedChargeTarget() const
{
    using namespace EnemyFlyingConfig;

    XMFLOAT3 pos = GetPosition();
    XMFLOAT3 playerPos = Player_GetPosition();

    XMVECTOR toPlayer = XMLoadFloat3(&playerPos) - XMLoadFloat3(&pos);
    float dist = XMVectorGetX(XMVector3Length(toPlayer));

    return dist < CHARGE_DISTANCE;
}

//======================================
// StateHover 実装
//======================================

void EnemyFlying::StateHover::Enter()
{
    m_StateTimer = 0.0f;
}

void EnemyFlying::StateHover::Update(float dt)
{
    m_StateTimer += dt;

    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);

    if (pOwner->m_pPhysics)
    {
        pOwner->m_pPhysics->Update(dt);
    }

    pOwner->UpdateDistanceAndFacing();
    pOwner->ApplyHoverForce(dt);
    pOwner->CircleAroundPlayer(dt);
    pOwner->ClampToTerrain();

    if (m_StateTimer > 1.5f)
    {
        float dist = pOwner->GetDistanceToPlayer();
        if (dist < EnemyFlyingConfig::ATTACK_RANGE)
        {
            pOwner->ChangeState(new StateWindup(pOwner));
        }
    }
}

void EnemyFlying::StateHover::Draw() const
{
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlying(pOwner->m_pPhysics);
}

void EnemyFlying::StateHover::DrawShadow() const
{
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlyingShadow(pOwner->m_pPhysics);
}

void EnemyFlying::StateHover::DrawDebug() const
{
#ifdef _DEBUG
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlyingDebug(pOwner->m_pPhysics, GetFlyingStateColor(this));
#endif
}

//======================================
// StateWindup 実装
//======================================

void EnemyFlying::StateWindup::Enter()
{
    m_StateTimer = 0.0f;

    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    pOwner->PrepareCharge();

    XMFLOAT3 vel = pOwner->GetVelocity();
    XMVECTOR vDir = XMLoadFloat3(&pOwner->GetChargeDirection());
    XMVECTOR vVel = vDir * -3.0f;
    XMStoreFloat3(&vel, vVel);
    pOwner->SetVelocity(vel);

    // 突撃準備SE
    SoundManager_PlaySE(SOUND_SE_FLYING_CHARGE_READY);
}

void EnemyFlying::StateWindup::Update(float dt)
{
    m_StateTimer += dt;

    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);

    if (pOwner->m_pPhysics)
    {
        pOwner->m_pPhysics->Update(dt);
    }

    pOwner->UpdateDistanceAndFacing();
    pOwner->PrepareCharge();
    pOwner->ClampToTerrain();

    XMFLOAT3 pos = pOwner->GetPosition();
    Trail_Create(pos, { 1.0f, 0.8f, 0.2f, 0.5f }, 0.3f, 0.2);

    if (m_StateTimer >= EnemyFlyingConfig::CHARGE_WINDUP_TIME)
    {
        pOwner->ChangeState(new StateCharge(pOwner));
    }
}

void EnemyFlying::StateWindup::Draw() const
{
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlying(pOwner->m_pPhysics);
}

void EnemyFlying::StateWindup::DrawShadow() const
{
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlyingShadow(pOwner->m_pPhysics);
}

void EnemyFlying::StateWindup::DrawDebug() const
{
#ifdef _DEBUG
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlyingDebug(pOwner->m_pPhysics, GetFlyingStateColor(this));

    XMFLOAT3 pos = pOwner->GetPosition();
    DebugRenderer::DrawLine(pos, pOwner->GetChargeTarget(), { 1.0f, 0.0f, 0.0f, 1.0f });
#endif
}

//======================================
// StateCharge 実装
//======================================

void EnemyFlying::StateCharge::Enter()
{
    m_StateTimer = 0.0f;

    // 突撃SE
    SoundManager_PlaySE(SOUND_SE_FLYING_CHARGE);
}

void EnemyFlying::StateCharge::Update(float dt)
{
    m_StateTimer += dt;

    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);

    if (pOwner->m_pPhysics)
    {
        pOwner->m_pPhysics->Update(dt);
    }

    pOwner->ExecuteCharge(dt);

    // 一定間隔で風切り音
    static float whooshTimer = 0.0f;
    whooshTimer += dt;
    if (whooshTimer >= 0.3f)
    {
        SoundManager_PlaySE(SOUND_SE_FLYING_WHOOSH);
        whooshTimer = 0.0f;
    }

    if (pOwner->HasReachedChargeTarget() ||
        m_StateTimer >= EnemyFlyingConfig::CHARGE_MAX_DURATION)
    {
        pOwner->ChangeState(new StateRetreat(pOwner));
    }
}

void EnemyFlying::StateCharge::Draw() const
{
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlying(pOwner->m_pPhysics);
}

void EnemyFlying::StateCharge::DrawShadow() const
{
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlyingShadow(pOwner->m_pPhysics);
}

void EnemyFlying::StateCharge::DrawDebug() const
{
#ifdef _DEBUG
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlyingDebug(pOwner->m_pPhysics, GetFlyingStateColor(this));
#endif
}

//======================================
// StateRetreat 実装
//======================================

void EnemyFlying::StateRetreat::Enter()
{
    using namespace EnemyFlyingConfig;

    m_StateTimer = 0.0f;

    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    XMFLOAT3 pos = pOwner->GetPosition();
    XMFLOAT3 playerPos = Player_GetPosition();

    XMVECTOR awayDir = XMLoadFloat3(&pos) - XMLoadFloat3(&playerPos);
    awayDir = XMVectorSetY(awayDir, 0.0f);
    awayDir = XMVector3Normalize(awayDir);

    XMFLOAT3 retreatPos;
    XMStoreFloat3(&retreatPos, XMLoadFloat3(&playerPos) + awayDir * RETREAT_DISTANCE);

    float terrainHeight = Stage_GetTerrainHeight(retreatPos.x, retreatPos.z);
    float targetHeight = std::max(playerPos.y + RETREAT_HEIGHT, terrainHeight + TERRAIN_MIN_HEIGHT + RETREAT_HEIGHT);

    XMVECTOR vTarget = XMLoadFloat3(&playerPos) + awayDir * RETREAT_DISTANCE;
    vTarget = XMVectorSetY(vTarget, targetHeight);
    XMStoreFloat3(&m_RetreatTarget, vTarget);

    XMFLOAT3 vel;
    XMStoreFloat3(&vel, awayDir * RETREAT_SPEED * 0.5f);
    vel.y = RETREAT_SPEED * 0.5f;
    pOwner->SetVelocity(vel);
}

void EnemyFlying::StateRetreat::Update(float dt)
{
    using namespace EnemyFlyingConfig;

    m_StateTimer += dt;

    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);

    if (pOwner->m_pPhysics)
    {
        pOwner->m_pPhysics->Update(dt);
    }

    pOwner->UpdateDistanceAndFacing();
    pOwner->MoveToward(m_RetreatTarget, RETREAT_SPEED * 5.0f, dt);
    pOwner->ClampToTerrain();

    XMFLOAT3 pos = pOwner->GetPosition();
    Trail_Create(pos, { 0.5f, 0.3f, 1.0f, 0.6f }, 0.4f, 0.25);

    if (m_StateTimer >= RETREAT_DURATION)
    {
        pOwner->ChangeState(new StateCooldown(pOwner));
    }
}

void EnemyFlying::StateRetreat::Draw() const
{
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlying(pOwner->m_pPhysics);
}

void EnemyFlying::StateRetreat::DrawShadow() const
{
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlyingShadow(pOwner->m_pPhysics);
}

void EnemyFlying::StateRetreat::DrawDebug() const
{
#ifdef _DEBUG
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlyingDebug(pOwner->m_pPhysics, GetFlyingStateColor(this));
#endif
}

//======================================
// StateCooldown 実装
//======================================

void EnemyFlying::StateCooldown::Enter()
{
    m_StateTimer = 0.0f;

    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    XMFLOAT3 vel = pOwner->GetVelocity();
    vel.x *= 0.3f;
    vel.y *= 0.3f;
    vel.z *= 0.3f;
    pOwner->SetVelocity(vel);
}

void EnemyFlying::StateCooldown::Update(float dt)
{
    m_StateTimer += dt;

    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);

    if (pOwner->m_pPhysics)
    {
        pOwner->m_pPhysics->Update(dt);
    }

    pOwner->UpdateDistanceAndFacing();
    pOwner->ApplyHoverForce(dt);
    pOwner->ClampToTerrain();

    pOwner->GetCircleAngle() += EnemyFlyingConfig::CIRCLE_SPEED * 0.5f * dt;

    if (m_StateTimer >= EnemyFlyingConfig::COOLDOWN_DURATION)
    {
        pOwner->ChangeState(new StateHover(pOwner));
    }
}

void EnemyFlying::StateCooldown::Draw() const
{
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlying(pOwner->m_pPhysics);
}

void EnemyFlying::StateCooldown::DrawShadow() const
{
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlyingShadow(pOwner->m_pPhysics);
}

void EnemyFlying::StateCooldown::DrawDebug() const
{
#ifdef _DEBUG
    EnemyFlying* pOwner = static_cast<EnemyFlying*>(m_pOwner);
    DrawEnemyFlyingDebug(pOwner->m_pPhysics, GetFlyingStateColor(this));
#endif
}

//======================================
// 切断結果からEnemyFlyingを生成
//======================================
EnemyFlying* EnemyFlying_CreateFromSlice(const SlicedFlyingEnemyParams& params)
{
    using namespace EnemyFlyingConfig;

    if (params.meshes.empty() || !params.originalModel)
    {
        return nullptr;
    }

    MODEL* newModel = ModelCreateFromData(params.meshes, params.originalModel);
    if (!newModel)
    {
        return nullptr;
    }

    EnemyFlying* pEnemy = new EnemyFlying(params.position);

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

    pEnemy->m_pPhysics->GetRigidBody()->SetGravityEnabled(false);

    RigidBody::Params rbParams = pEnemy->m_pPhysics->GetRigidBody()->GetParams();
    rbParams.linearDrag = DRAG;
    pEnemy->m_pPhysics->GetRigidBody()->SetParams(rbParams);

    pEnemy->m_pPhysics->SetRootVolume(params.rootVolume);

    pEnemy->m_pPhysics->SetIgnoreCollisionTimer(SLICE_IMMUNITY_TIME);

    XMFLOAT3 torque;
    XMStoreFloat3(&torque, vNormal * (SLICE_TORQUE_STRENGTH * sign));
    pEnemy->m_pPhysics->GetRigidBody()->AddTorque(torque);

    pEnemy->m_Position = newPosition;

    ModelRelease(newModel);

    // 切断SE（飛行敵も同じSEを使用）
    SoundManager_PlaySE(SOUND_SE_ENEMY_SLICE);

    return pEnemy;
}