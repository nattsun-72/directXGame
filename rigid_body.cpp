/****************************************
 * @file rigid_body.cpp
 * @brief 物理挙動コンポーネント
 * @author Natsume Shidara
 * @update 2026/01/06 - リファクタリング
 ****************************************/

#include "rigid_body.h"
#include "collision.h"
#include <cmath>
#include <algorithm>
#include <cfloat>

using namespace DirectX;

namespace {
    XMVECTOR ClampMagnitude(XMVECTOR v, float maxLen)
    {
        float lenSq = XMVectorGetX(XMVector3LengthSq(v));
        if (lenSq > maxLen * maxLen)
        {
            return XMVectorScale(XMVector3Normalize(v), maxLen);
        }
        return v;
    }

    float SafeInverse(float value)
    {
        return (value > RigidBody::MIN_MASS) ? 1.0f / value : 0.0f;
    }
}

//======================================
// コンストラクタ
//======================================
RigidBody::RigidBody()
    : m_Params()
    , m_Position{ 0.0f, 0.0f, 0.0f }
    , m_Rotation()
    , m_Velocity{ 0.0f, 0.0f, 0.0f }
    , m_AngularVelocity{ 0.0f, 0.0f, 0.0f }
    , m_ForceAccum{ 0.0f, 0.0f, 0.0f }
    , m_TorqueAccum{ 0.0f, 0.0f, 0.0f }
    , m_LocalCollider()
    , m_LocalAABB()
    , m_WorldMatrix()
    , m_InvInertiaTensorLocal()
    , m_InvInertiaTensorWorld()
    , m_Constraints(RigidbodyConstraints::None)
    , m_IsSleeping(false)
    , m_SleepTimer(0.0f)
    , m_HasGroundContact(false)
    , m_GenerationId(0)
    , m_IgnoreCollisionTimer(0.0f)
{
    XMStoreFloat4(&m_Rotation, XMQuaternionIdentity());
    XMStoreFloat4x4(&m_WorldMatrix, XMMatrixIdentity());
    XMStoreFloat3x3(&m_InvInertiaTensorLocal, XMMatrixIdentity());
    XMStoreFloat3x3(&m_InvInertiaTensorWorld, XMMatrixIdentity());

    m_LocalCollider.type = ColliderType::Sphere;
    m_LocalCollider.sphere.center = { 0.0f, 0.0f, 0.0f };
    m_LocalCollider.sphere.radius = 0.5f;
    m_LocalAABB = { { -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f } };
}

//======================================
// 初期化
//======================================
void RigidBody::Initialize(const XMFLOAT3& pos, const Collider& collider, float mass)
{
    m_Position = pos;
    m_LocalCollider = collider;
    m_Params.mass = std::max(mass, MIN_MASS);

    // OBBの場合：向きを初期回転に、ローカルは回転なしに
    if (collider.type == ColliderType::Box)
    {
        m_Rotation = collider.obb.orientation;
        XMStoreFloat4(&m_LocalCollider.obb.orientation, XMQuaternionIdentity());
    }
    else
    {
        XMStoreFloat4(&m_Rotation, XMQuaternionIdentity());
    }

    ResetVelocity();
    WakeUp();

    // ローカルAABBの計算
    if (collider.type == ColliderType::Sphere)
    {
        const float r = collider.sphere.radius;
        const XMFLOAT3& c = collider.sphere.center;
        m_LocalAABB.min = { c.x - r, c.y - r, c.z - r };
        m_LocalAABB.max = { c.x + r, c.y + r, c.z + r };
    }
    else if (collider.type == ColliderType::Box)
    {
        const XMFLOAT3& c = collider.obb.center;
        const XMFLOAT3& e = collider.obb.extents;
        XMMATRIX rot = XMMatrixRotationQuaternion(XMLoadFloat4(&collider.obb.orientation));
        XMVECTOR vCenter = XMLoadFloat3(&c);
        XMVECTOR vMin = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
        XMVECTOR vMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);

        for (int i = 0; i < 8; ++i)
        {
            XMVECTOR corner = XMVectorSet(
                (i & 1) ? e.x : -e.x,
                (i & 2) ? e.y : -e.y,
                (i & 4) ? e.z : -e.z,
                0.0f);
            corner = XMVector3TransformNormal(corner, rot) + vCenter;
            vMin = XMVectorMin(vMin, corner);
            vMax = XMVectorMax(vMax, corner);
        }
        XMStoreFloat3(&m_LocalAABB.min, vMin);
        XMStoreFloat3(&m_LocalAABB.max, vMax);
    }

    // 慣性テンソルの計算
    float Ixx = 0.0f, Iyy = 0.0f, Izz = 0.0f;
    XMFLOAT3 offset = { 0.0f, 0.0f, 0.0f };

    if (collider.type == ColliderType::Sphere)
    {
        offset = collider.sphere.center;
        const float r = std::max(collider.sphere.radius, 0.01f);
        const float I = (2.0f / 5.0f) * m_Params.mass * r * r;
        Ixx = Iyy = Izz = I;
    }
    else if (collider.type == ColliderType::Box)
    {
        offset = collider.obb.center;
        const float w = std::max(collider.obb.extents.x * 2.0f, 0.01f);
        const float h = std::max(collider.obb.extents.y * 2.0f, 0.01f);
        const float d = std::max(collider.obb.extents.z * 2.0f, 0.01f);
        const float coef = m_Params.mass / 12.0f;
        Ixx = coef * (h * h + d * d);
        Iyy = coef * (w * w + d * d);
        Izz = coef * (w * w + h * h);
    }

    // 平行軸の定理
    const float offsetLenSq = offset.x * offset.x + offset.y * offset.y + offset.z * offset.z;
    if (offsetLenSq > 1e-6f)
    {
        Ixx += m_Params.mass * (offset.y * offset.y + offset.z * offset.z);
        Iyy += m_Params.mass * (offset.x * offset.x + offset.z * offset.z);
        Izz += m_Params.mass * (offset.x * offset.x + offset.y * offset.y);
    }

    const float minInertia = MIN_INERTIA_FACTOR * m_Params.mass;
    Ixx = std::max(Ixx, minInertia);
    Iyy = std::max(Iyy, minInertia);
    Izz = std::max(Izz, minInertia);

    XMMATRIX invInertia = XMMatrixIdentity();
    invInertia.r[0] = XMVectorSet(1.0f / Ixx, 0.0f, 0.0f, 0.0f);
    invInertia.r[1] = XMVectorSet(0.0f, 1.0f / Iyy, 0.0f, 0.0f);
    invInertia.r[2] = XMVectorSet(0.0f, 0.0f, 1.0f / Izz, 0.0f);
    XMStoreFloat3x3(&m_InvInertiaTensorLocal, invInertia);

    UpdateWorldMatrix();
    UpdateInertiaTensor();
}

//======================================
// 物理更新
//======================================
void RigidBody::Integrate(float dt)
{
    if (dt <= 0.0f) return;
    dt = std::min(dt, MAX_DELTA_TIME);

    if (m_IgnoreCollisionTimer > 0.0f)
    {
        m_IgnoreCollisionTimer = std::max(m_IgnoreCollisionTimer - dt, 0.0f);
    }

    if (m_Params.isKinematic || m_IsSleeping)
    {
        m_ForceAccum = { 0.0f, 0.0f, 0.0f };
        m_TorqueAccum = { 0.0f, 0.0f, 0.0f };
        m_HasGroundContact = false;
        return;
    }

    ApplyConstraintsToForces();

    // 重力適用
    if (m_Params.useGravity)
    {
        XMVECTOR vGravity = XMLoadFloat3(&m_Params.gravity);
        XMVECTOR vForce = XMVectorScale(vGravity, m_Params.mass);
        XMVECTOR vAccum = XMLoadFloat3(&m_ForceAccum);
        XMStoreFloat3(&m_ForceAccum, XMVectorAdd(vAccum, vForce));
    }

    XMVECTOR vPos = XMLoadFloat3(&m_Position);
    XMVECTOR qRot = XMLoadFloat4(&m_Rotation);
    XMVECTOR vVel = XMLoadFloat3(&m_Velocity);
    XMVECTOR vAngVel = XMLoadFloat3(&m_AngularVelocity);
    XMVECTOR vForce = XMLoadFloat3(&m_ForceAccum);
    XMVECTOR vTorque = XMLoadFloat3(&m_TorqueAccum);

    // 加速度計算
    const float invMass = SafeInverse(m_Params.mass);
    XMVECTOR vAcc = XMVectorScale(vForce, invMass);

    XMMATRIX R = XMMatrixRotationQuaternion(qRot);
    XMMATRIX invILocal = XMLoadFloat3x3(&m_InvInertiaTensorLocal);
    XMMATRIX invInertiaWorld = XMMatrixMultiply(XMMatrixMultiply(R, invILocal), XMMatrixTranspose(R));
    XMVECTOR vAngAcc = XMVector3TransformNormal(vTorque, invInertiaWorld);

    // 速度更新
    vVel = XMVectorAdd(vVel, XMVectorScale(vAcc, dt));
    vAngVel = XMVectorAdd(vAngVel, XMVectorScale(vAngAcc, dt));

    // 減衰
    const float linearDamp = std::exp(-m_Params.linearDrag * dt);
    const float angularDamp = std::exp(-m_Params.angularDrag * dt);
    vVel = XMVectorScale(vVel, linearDamp);
    vAngVel = XMVectorScale(vAngVel, angularDamp);

    // 微小速度カット（接地時のみ線形速度をカット）
    float velLenSq = XMVectorGetX(XMVector3LengthSq(vVel));
    float angVelLenSq = XMVectorGetX(XMVector3LengthSq(vAngVel));

    if (m_HasGroundContact && velLenSq < VELOCITY_CUTOFF)
    {
        vVel = XMVectorZero();
    }
    if (angVelLenSq < VELOCITY_CUTOFF)
    {
        vAngVel = XMVectorZero();
    }

    // 速度制限
    vVel = ClampMagnitude(vVel, m_Params.maxLinearVelocity);
    vAngVel = ClampMagnitude(vAngVel, m_Params.maxAngularVelocity);

    ApplyConstraintsToVelocities(vVel, vAngVel);

    // 位置・回転更新
    vPos = XMVectorAdd(vPos, XMVectorScale(vVel, dt));
    qRot = UpdateRotation(qRot, vAngVel, dt);

    XMStoreFloat3(&m_Position, vPos);
    XMStoreFloat4(&m_Rotation, qRot);
    XMStoreFloat3(&m_Velocity, vVel);
    XMStoreFloat3(&m_AngularVelocity, vAngVel);

    UpdateWorldMatrix();
    UpdateInertiaTensor();
    CheckSleepState(dt);

    m_ForceAccum = { 0.0f, 0.0f, 0.0f };
    m_TorqueAccum = { 0.0f, 0.0f, 0.0f };
    m_HasGroundContact = false;  // 毎フレームリセット
}

//======================================
// 回転更新
//======================================
XMVECTOR RigidBody::UpdateRotation(XMVECTOR qRot, XMVECTOR vAngVel, float dt) const
{
    float angVelLenSq = XMVectorGetX(XMVector3LengthSq(vAngVel));
    if (angVelLenSq > 1e-8f)
    {
        float angVelLen = std::sqrt(angVelLenSq);
        float theta = std::min(angVelLen * dt, MAX_ROTATION_PER_FRAME);
        XMVECTOR vAxis = XMVectorScale(vAngVel, 1.0f / angVelLen);
        XMVECTOR qDelta = XMQuaternionRotationAxis(vAxis, theta);
        qRot = XMQuaternionNormalize(XMQuaternionMultiply(qDelta, qRot));
    }
    return qRot;
}

//======================================
// 拘束条件
//======================================
void RigidBody::ApplyConstraintsToForces()
{
    if ((m_Constraints & RigidbodyConstraints::FreezePositionX) != RigidbodyConstraints::None)
        m_ForceAccum.x = 0.0f;
    if ((m_Constraints & RigidbodyConstraints::FreezePositionY) != RigidbodyConstraints::None)
        m_ForceAccum.y = 0.0f;
    if ((m_Constraints & RigidbodyConstraints::FreezePositionZ) != RigidbodyConstraints::None)
        m_ForceAccum.z = 0.0f;
    if ((m_Constraints & RigidbodyConstraints::FreezeRotationX) != RigidbodyConstraints::None)
        m_TorqueAccum.x = 0.0f;
    if ((m_Constraints & RigidbodyConstraints::FreezeRotationY) != RigidbodyConstraints::None)
        m_TorqueAccum.y = 0.0f;
    if ((m_Constraints & RigidbodyConstraints::FreezeRotationZ) != RigidbodyConstraints::None)
        m_TorqueAccum.z = 0.0f;
}

void RigidBody::ApplyConstraintsToVelocities(XMVECTOR& vVel, XMVECTOR& vAngVel)
{
    XMFLOAT3 fVel, fAngVel;
    XMStoreFloat3(&fVel, vVel);
    XMStoreFloat3(&fAngVel, vAngVel);

    if ((m_Constraints & RigidbodyConstraints::FreezePositionX) != RigidbodyConstraints::None) fVel.x = 0.0f;
    if ((m_Constraints & RigidbodyConstraints::FreezePositionY) != RigidbodyConstraints::None) fVel.y = 0.0f;
    if ((m_Constraints & RigidbodyConstraints::FreezePositionZ) != RigidbodyConstraints::None) fVel.z = 0.0f;
    if ((m_Constraints & RigidbodyConstraints::FreezeRotationX) != RigidbodyConstraints::None) fAngVel.x = 0.0f;
    if ((m_Constraints & RigidbodyConstraints::FreezeRotationY) != RigidbodyConstraints::None) fAngVel.y = 0.0f;
    if ((m_Constraints & RigidbodyConstraints::FreezeRotationZ) != RigidbodyConstraints::None) fAngVel.z = 0.0f;

    vVel = XMLoadFloat3(&fVel);
    vAngVel = XMLoadFloat3(&fAngVel);
}

//======================================
// 衝突解決
//======================================
bool RigidBody::SolveCollision(RigidBody* rbA, RigidBody* rbB)
{
    if (!rbA || !rbB) return false;
    if (rbA->m_Params.isKinematic && rbB->m_Params.isKinematic) return false;

    if (rbA->m_GenerationId != 0 && rbA->m_GenerationId == rbB->m_GenerationId)
    {
        if (rbA->m_IgnoreCollisionTimer > 0.0f || rbB->m_IgnoreCollisionTimer > 0.0f)
            return false;
    }

    const Collider colA = rbA->GetWorldCollider();
    const Collider colB = rbB->GetWorldCollider();
    Hit hit = Collision_Detect(colA, colB);

    if (!hit.isHit) return false;

    if (hit.depth > 0.5f)
    {
        ApplyPositionalCorrection(rbA, rbB, hit, SafeInverse(rbA->m_Params.mass) + SafeInverse(rbB->m_Params.mass));
        return true;
    }

    return ApplyCollisionImpulse(rbA, rbB, hit);
}

bool RigidBody::ApplyCollisionImpulse(RigidBody* rbA, RigidBody* rbB, const Hit& hit)
{
    XMVECTOR n = XMVectorNegate(XMLoadFloat3(&hit.normal));
    XMVECTOR contact = XMLoadFloat3(&hit.contactPoint);

    const Params& pA = rbA->m_Params;
    const Params& pB = rbB->m_Params;

    float invMassA = (pA.isKinematic || pA.mass <= 0.0f) ? 0.0f : 1.0f / pA.mass;
    float invMassB = (pB.isKinematic || pB.mass <= 0.0f) ? 0.0f : 1.0f / pB.mass;
    float totalInvMass = invMassA + invMassB;
    if (totalInvMass <= 0.0f) return true;

    XMMATRIX invInertiaA = pA.isKinematic ? XMMatrixSet(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) : XMLoadFloat3x3(&rbA->m_InvInertiaTensorWorld);
    XMMATRIX invInertiaB = pB.isKinematic ? XMMatrixSet(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) : XMLoadFloat3x3(&rbB->m_InvInertiaTensorWorld);

    XMVECTOR posA = XMLoadFloat3(&rbA->m_Position);
    XMVECTOR posB = XMLoadFloat3(&rbB->m_Position);
    XMVECTOR rA = XMVectorSubtract(contact, posA);
    XMVECTOR rB = XMVectorSubtract(contact, posB);

    float rA_len = XMVectorGetX(XMVector3Length(rA));
    float rB_len = XMVectorGetX(XMVector3Length(rB));
    if (rA_len > MAX_ARM_LENGTH) rA = XMVectorScale(XMVector3Normalize(rA), MAX_ARM_LENGTH);
    if (rB_len > MAX_ARM_LENGTH) rB = XMVectorScale(XMVector3Normalize(rB), MAX_ARM_LENGTH);

    XMVECTOR vA = XMLoadFloat3(&rbA->m_Velocity);
    XMVECTOR wA = XMLoadFloat3(&rbA->m_AngularVelocity);
    XMVECTOR vB = XMLoadFloat3(&rbB->m_Velocity);
    XMVECTOR wB = XMLoadFloat3(&rbB->m_AngularVelocity);

    XMVECTOR vPointA = XMVectorAdd(vA, XMVector3Cross(wA, rA));
    XMVECTOR vPointB = XMVectorAdd(vB, XMVector3Cross(wB, rB));
    XMVECTOR vRel = XMVectorSubtract(vPointB, vPointA);

    float velAlongNormal = XMVectorGetX(XMVector3Dot(vRel, n));
    if (velAlongNormal > 0.0f) return true;

    float e = std::min(pA.restitution, pB.restitution);
    if (std::abs(velAlongNormal) < 1.0f) e *= 0.5f;

    float j = -(1.0f + e) * velAlongNormal;

    XMVECTOR rA_cross_n = XMVector3Cross(rA, n);
    XMVECTOR rB_cross_n = XMVector3Cross(rB, n);
    XMVECTOR termA = XMVector3TransformNormal(rA_cross_n, invInertiaA);
    XMVECTOR termB = XMVector3TransformNormal(rB_cross_n, invInertiaB);
    float rotTermA = XMVectorGetX(XMVector3Dot(termA, rA_cross_n));
    float rotTermB = XMVectorGetX(XMVector3Dot(termB, rB_cross_n));

    j /= (totalInvMass + rotTermA + rotTermB);
    j = std::min(j, MAX_IMPULSE);

    XMVECTOR impulse = XMVectorScale(n, j);
    XMFLOAT3 fImpulse, fNegImpulse, fContact;
    XMStoreFloat3(&fImpulse, impulse);
    XMStoreFloat3(&fNegImpulse, XMVectorNegate(impulse));
    XMStoreFloat3(&fContact, contact);

    rbA->ApplyImpulseAtPoint(fNegImpulse, fContact);
    rbB->ApplyImpulseAtPoint(fImpulse, fContact);

    // 摩擦
    vPointA = XMVectorAdd(XMLoadFloat3(&rbA->m_Velocity), XMVector3Cross(XMLoadFloat3(&rbA->m_AngularVelocity), rA));
    vPointB = XMVectorAdd(XMLoadFloat3(&rbB->m_Velocity), XMVector3Cross(XMLoadFloat3(&rbB->m_AngularVelocity), rB));
    vRel = XMVectorSubtract(vPointB, vPointA);

    XMVECTOR tangent = XMVectorSubtract(vRel, XMVectorScale(n, XMVectorGetX(XMVector3Dot(vRel, n))));
    float tangentLenSq = XMVectorGetX(XMVector3LengthSq(tangent));

    if (tangentLenSq > 1e-6f)
    {
        tangent = XMVector3Normalize(tangent);
        float jt = -XMVectorGetX(XMVector3Dot(vRel, tangent));

        XMVECTOR rA_cross_t = XMVector3Cross(rA, tangent);
        XMVECTOR rB_cross_t = XMVector3Cross(rB, tangent);
        XMVECTOR termAt = XMVector3TransformNormal(rA_cross_t, invInertiaA);
        XMVECTOR termBt = XMVector3TransformNormal(rB_cross_t, invInertiaB);
        float rotTermAt = XMVectorGetX(XMVector3Dot(termAt, rA_cross_t));
        float rotTermBt = XMVectorGetX(XMVector3Dot(termBt, rB_cross_t));

        jt /= (totalInvMass + rotTermAt + rotTermBt);

        float mu = std::sqrt(pA.friction * pB.friction);
        if (std::abs(jt) > j * mu)
            jt = (jt > 0.0f ? 1.0f : -1.0f) * j * mu;

        XMVECTOR fricImpulse = XMVectorScale(tangent, jt);
        XMFLOAT3 fFric, fNegFric;
        XMStoreFloat3(&fFric, fricImpulse);
        XMStoreFloat3(&fNegFric, XMVectorNegate(fricImpulse));

        rbA->ApplyImpulseAtPoint(fNegFric, fContact);
        rbB->ApplyImpulseAtPoint(fFric, fContact);
    }

    ApplyPositionalCorrection(rbA, rbB, hit, totalInvMass);
    return true;
}

void RigidBody::ApplyPositionalCorrection(RigidBody* rbA, RigidBody* rbB, const Hit& hit, float totalInvMass)
{
    float correctionDepth = std::max(hit.depth - CORRECTION_SLOP, 0.0f);
    if (correctionDepth <= 0.0f) return;

    XMVECTOR n = XMVectorNegate(XMLoadFloat3(&hit.normal));
    XMVECTOR posA = XMLoadFloat3(&rbA->m_Position);
    XMVECTOR posB = XMLoadFloat3(&rbB->m_Position);

    float invMassA = (rbA->m_Params.isKinematic || rbA->m_Params.mass <= 0.0f) ? 0.0f : 1.0f / rbA->m_Params.mass;
    float invMassB = (rbB->m_Params.isKinematic || rbB->m_Params.mass <= 0.0f) ? 0.0f : 1.0f / rbB->m_Params.mass;

    XMVECTOR correction = XMVectorScale(n, correctionDepth / totalInvMass * CORRECTION_PERCENT);
    XMVECTOR posA_new = XMVectorSubtract(posA, XMVectorScale(correction, invMassA));
    XMVECTOR posB_new = XMVectorAdd(posB, XMVectorScale(correction, invMassB));

    XMFLOAT3 fPosA, fPosB;
    XMStoreFloat3(&fPosA, posA_new);
    XMStoreFloat3(&fPosB, posB_new);

    if (!rbA->m_Params.isKinematic) rbA->SetPosition(fPosA);
    if (!rbB->m_Params.isKinematic) rbB->SetPosition(fPosB);
}

//======================================
// スリープ管理
//======================================
void RigidBody::CheckSleepState(float dt)
{
    // 接地していない場合はスリープしない
    if (!m_HasGroundContact)
    {
        m_SleepTimer = 0.0f;
        m_IsSleeping = false;
        return;
    }

    float velSq = XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&m_Velocity)));
    float angVelSq = XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&m_AngularVelocity)));
    float energy = velSq + angVelSq;

    if (energy < m_Params.sleepThreshold * SLEEP_ENERGY_BIAS)
    {
        m_SleepTimer += dt;
        if (m_SleepTimer > SLEEP_TIME_THRESHOLD)
        {
            m_IsSleeping = true;
            ResetVelocity();
        }
    }
    else
    {
        m_SleepTimer = 0.0f;
        m_IsSleeping = false;
    }
}

void RigidBody::WakeUp()
{
    m_IsSleeping = false;
    m_SleepTimer = 0.0f;
}

void RigidBody::NotifyGroundContact()
{
    m_HasGroundContact = true;
}

void RigidBody::ResetVelocity()
{
    m_Velocity = { 0.0f, 0.0f, 0.0f };
    m_AngularVelocity = { 0.0f, 0.0f, 0.0f };
    m_ForceAccum = { 0.0f, 0.0f, 0.0f };
    m_TorqueAccum = { 0.0f, 0.0f, 0.0f };
}

//======================================
// 力・インパルス
//======================================
void RigidBody::AddForce(const XMFLOAT3& force)
{
    if (m_Params.isKinematic) return;
    WakeUp();
    XMVECTOR vForce = XMLoadFloat3(&force);
    XMVECTOR vAccum = XMLoadFloat3(&m_ForceAccum);
    XMStoreFloat3(&m_ForceAccum, XMVectorAdd(vAccum, vForce));
}

void RigidBody::AddForceAtPoint(const XMFLOAT3& force, const XMFLOAT3& point)
{
    if (m_Params.isKinematic) return;
    WakeUp();

    XMVECTOR vForce = XMLoadFloat3(&force);
    XMVECTOR vPoint = XMLoadFloat3(&point);
    XMVECTOR vPos = XMLoadFloat3(&m_Position);
    XMVECTOR r = XMVectorSubtract(vPoint, vPos);
    XMVECTOR vTorque = XMVector3Cross(r, vForce);
    XMVECTOR vTorqueAccum = XMLoadFloat3(&m_TorqueAccum);
    XMStoreFloat3(&m_TorqueAccum, XMVectorAdd(vTorqueAccum, vTorque));

    AddForce(force);
}

void RigidBody::AddTorque(const XMFLOAT3& torque)
{
    if (m_Params.isKinematic) return;
    WakeUp();
    XMVECTOR vTorque = XMLoadFloat3(&torque);
    XMVECTOR vAccum = XMLoadFloat3(&m_TorqueAccum);
    XMStoreFloat3(&m_TorqueAccum, XMVectorAdd(vAccum, vTorque));
}

void RigidBody::ApplyImpulse(const XMFLOAT3& impulse)
{
    if (m_Params.isKinematic) return;
    WakeUp();
    XMVECTOR vImpulse = XMLoadFloat3(&impulse);
    XMVECTOR vVel = XMLoadFloat3(&m_Velocity);
    vVel = XMVectorAdd(vVel, XMVectorScale(vImpulse, SafeInverse(m_Params.mass)));
    vVel = ClampMagnitude(vVel, m_Params.maxLinearVelocity);
    XMStoreFloat3(&m_Velocity, vVel);
}

void RigidBody::ApplyImpulseAtPoint(const XMFLOAT3& impulse, const XMFLOAT3& point)
{
    if (m_Params.isKinematic) return;
    ApplyImpulse(impulse);

    XMVECTOR vImpulse = XMLoadFloat3(&impulse);
    XMVECTOR vPoint = XMLoadFloat3(&point);
    XMVECTOR vPos = XMLoadFloat3(&m_Position);
    XMVECTOR vAngVel = XMLoadFloat3(&m_AngularVelocity);

    XMVECTOR r = XMVectorSubtract(vPoint, vPos);
    XMVECTOR impulsiveTorque = XMVector3Cross(r, vImpulse);
    XMMATRIX invInertiaWorld = XMLoadFloat3x3(&m_InvInertiaTensorWorld);
    XMVECTOR deltaAngVel = XMVector3TransformNormal(impulsiveTorque, invInertiaWorld);

    vAngVel = XMVectorAdd(vAngVel, deltaAngVel);
    vAngVel = ClampMagnitude(vAngVel, m_Params.maxAngularVelocity);
    XMStoreFloat3(&m_AngularVelocity, vAngVel);
}

//======================================
// Setters
//======================================
void RigidBody::SetVelocity(const XMFLOAT3& vel)
{
    m_Velocity = vel;
    if (std::abs(vel.x) > 0.0f || std::abs(vel.y) > 0.0f || std::abs(vel.z) > 0.0f)
        WakeUp();
}

void RigidBody::SetAngularVelocity(const XMFLOAT3& angVel)
{
    m_AngularVelocity = angVel;
    if (std::abs(angVel.x) > 0.0f || std::abs(angVel.y) > 0.0f || std::abs(angVel.z) > 0.0f)
        WakeUp();
}

void RigidBody::SetParams(const Params& param)
{
    bool massChanged = std::abs(m_Params.mass - param.mass) > 1e-6f;
    m_Params = param;
    if (massChanged) UpdateInertiaTensor();
    WakeUp();
}

void RigidBody::SetPosition(const XMFLOAT3& pos)
{
    m_Position = pos;
    UpdateWorldMatrix();
    WakeUp();
}

void RigidBody::SetRotation(const XMFLOAT4& rot)
{
    m_Rotation = rot;
    UpdateWorldMatrix();
    UpdateInertiaTensor();
    WakeUp();
}

//======================================
// 内部更新
//======================================
void RigidBody::UpdateInertiaTensor()
{
    XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&m_Rotation));
    XMMATRIX invILocal = XMLoadFloat3x3(&m_InvInertiaTensorLocal);
    XMMATRIX invIWorld = XMMatrixMultiply(XMMatrixMultiply(R, invILocal), XMMatrixTranspose(R));
    XMStoreFloat3x3(&m_InvInertiaTensorWorld, invIWorld);
}

void RigidBody::UpdateWorldMatrix()
{
    XMVECTOR vPos = XMLoadFloat3(&m_Position);
    XMVECTOR qRot = XMLoadFloat4(&m_Rotation);
    XMMATRIX mRot = XMMatrixRotationQuaternion(qRot);
    XMMATRIX mTrans = XMMatrixTranslationFromVector(vPos);
    XMStoreFloat4x4(&m_WorldMatrix, XMMatrixMultiply(mRot, mTrans));
}

//======================================
// Getters
//======================================
XMFLOAT4X4 RigidBody::GetWorldMatrix(const XMFLOAT3& scale) const
{
    XMMATRIX mWorld = XMLoadFloat4x4(&m_WorldMatrix);
    XMMATRIX mScale = XMMatrixScaling(scale.x, scale.y, scale.z);
    XMFLOAT4X4 result;
    XMStoreFloat4x4(&result, XMMatrixMultiply(mScale, mWorld));
    return result;
}

AABB RigidBody::GetTransformedAABB() const
{
    const XMVECTOR corners[8] = {
        XMVectorSet(m_LocalAABB.min.x, m_LocalAABB.min.y, m_LocalAABB.min.z, 0.0f),
        XMVectorSet(m_LocalAABB.max.x, m_LocalAABB.min.y, m_LocalAABB.min.z, 0.0f),
        XMVectorSet(m_LocalAABB.min.x, m_LocalAABB.max.y, m_LocalAABB.min.z, 0.0f),
        XMVectorSet(m_LocalAABB.min.x, m_LocalAABB.min.y, m_LocalAABB.max.z, 0.0f),
        XMVectorSet(m_LocalAABB.max.x, m_LocalAABB.max.y, m_LocalAABB.min.z, 0.0f),
        XMVectorSet(m_LocalAABB.max.x, m_LocalAABB.min.y, m_LocalAABB.max.z, 0.0f),
        XMVectorSet(m_LocalAABB.min.x, m_LocalAABB.max.y, m_LocalAABB.max.z, 0.0f),
        XMVectorSet(m_LocalAABB.max.x, m_LocalAABB.max.y, m_LocalAABB.max.z, 0.0f)
    };

    XMMATRIX worldMat = XMLoadFloat4x4(&m_WorldMatrix);
    XMVECTOR vMin = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
    XMVECTOR vMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);

    for (const auto& corner : corners)
    {
        XMVECTOR transformed = XMVector3Transform(corner, worldMat);
        vMin = XMVectorMin(vMin, transformed);
        vMax = XMVectorMax(vMax, transformed);
    }

    AABB result;
    XMStoreFloat3(&result.min, vMin);
    XMStoreFloat3(&result.max, vMax);
    return result;
}

Collider RigidBody::GetWorldCollider() const
{
    Collider worldCol = m_LocalCollider;
    XMVECTOR vPos = XMLoadFloat3(&m_Position);
    XMVECTOR qRot = XMLoadFloat4(&m_Rotation);

    if (worldCol.type == ColliderType::Sphere)
    {
        XMVECTOR vLocalCenter = XMLoadFloat3(&m_LocalCollider.sphere.center);
        XMVECTOR vWorldCenter = XMVectorAdd(XMVector3Rotate(vLocalCenter, qRot), vPos);
        XMStoreFloat3(&worldCol.sphere.center, vWorldCenter);
    }
    else if (worldCol.type == ColliderType::Box)
    {
        XMVECTOR vLocalCenter = XMLoadFloat3(&m_LocalCollider.obb.center);
        XMVECTOR vWorldCenter = XMVectorAdd(XMVector3Rotate(vLocalCenter, qRot), vPos);
        XMStoreFloat3(&worldCol.obb.center, vWorldCenter);

        XMVECTOR qLocalRot = XMLoadFloat4(&m_LocalCollider.obb.orientation);
        XMVECTOR qWorldRot = XMQuaternionMultiply(qLocalRot, qRot);
        XMStoreFloat4(&worldCol.obb.orientation, qWorldRot);
    }

    return worldCol;
}