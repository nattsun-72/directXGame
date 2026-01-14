/****************************************
 * @file rigid_body.h
 * @brief 物理挙動コンポーネント
 * @detail 剛体の物理シミュレーション、衝突応答機能を提供
 * @author Natsume Shidara
 * @update 2026/01/06 - リファクタリング
 ****************************************/

#ifndef RIGID_BODY_H
#define RIGID_BODY_H

#include <DirectXMath.h>
#include "collider.h"

 // 軸の拘束フラグ
enum class RigidbodyConstraints
{
    None = 0,
    FreezePositionX = 1 << 0,
    FreezePositionY = 1 << 1,
    FreezePositionZ = 1 << 2,
    FreezeRotationX = 1 << 3,
    FreezeRotationY = 1 << 4,
    FreezeRotationZ = 1 << 5,
    FreezePosition = FreezePositionX | FreezePositionY | FreezePositionZ,
    FreezeRotation = FreezeRotationX | FreezeRotationY | FreezeRotationZ,
    FreezeAll = 0xFF
};

inline RigidbodyConstraints operator|(RigidbodyConstraints a, RigidbodyConstraints b)
{
    return static_cast<RigidbodyConstraints>(static_cast<int>(a) | static_cast<int>(b));
}

inline RigidbodyConstraints operator&(RigidbodyConstraints a, RigidbodyConstraints b)
{
    return static_cast<RigidbodyConstraints>(static_cast<int>(a) & static_cast<int>(b));
}

class RigidBody
{
public:
    // 物理パラメータ構造体
    struct Params
    {
        float mass = 1.0f;
        float linearDrag = 0.5f;
        float angularDrag = 3.0f;
        bool isKinematic = false;
        DirectX::XMFLOAT3 gravity = { 0.0f, -9.81f, 0.0f };

        float maxLinearVelocity = 20.0f;
        float maxAngularVelocity = 10.0f;
        float sleepThreshold = 0.05f;

        float restitution = 0.2f;
        float friction = 0.6f;  // 修正: 3.8 → 0.6

        bool useGravity = true;
    };

public:
    RigidBody();
    ~RigidBody() = default;

    RigidBody(const RigidBody&) = default;
    RigidBody& operator=(const RigidBody&) = default;
    RigidBody(RigidBody&&) = default;
    RigidBody& operator=(RigidBody&&) = default;

    // 初期化・更新
    void Initialize(const DirectX::XMFLOAT3& pos, const Collider& collider, float mass);
    void Integrate(float dt);

    // 衝突解決
    static bool SolveCollision(RigidBody* rbA, RigidBody* rbB);

    // 力・インパルスの適用
    void AddForce(const DirectX::XMFLOAT3& force);
    void AddForceAtPoint(const DirectX::XMFLOAT3& force, const DirectX::XMFLOAT3& point);
    void AddTorque(const DirectX::XMFLOAT3& torque);
    void ApplyImpulse(const DirectX::XMFLOAT3& impulse);
    void ApplyImpulseAtPoint(const DirectX::XMFLOAT3& impulse, const DirectX::XMFLOAT3& point);

    // 状態制御
    void ResetVelocity();
    void WakeUp();
    void NotifyGroundContact();  // 接地通知（スリープ判定用）

    // Setters
    void SetPosition(const DirectX::XMFLOAT3& pos);
    void SetRotation(const DirectX::XMFLOAT4& rot);
    void SetVelocity(const DirectX::XMFLOAT3& vel);
    void SetAngularVelocity(const DirectX::XMFLOAT3& angVel);
    void SetConstraints(RigidbodyConstraints constraints) { m_Constraints = constraints; }
    void SetParams(const Params& param);
    void SetGenerationId(int id) { m_GenerationId = id; }
    void SetIgnoreCollisionTimer(float time) { m_IgnoreCollisionTimer = time; }

    // Getters
    const DirectX::XMFLOAT3& GetPosition() const { return m_Position; }
    const DirectX::XMFLOAT4& GetRotation() const { return m_Rotation; }
    const DirectX::XMFLOAT3& GetVelocity() const { return m_Velocity; }
    const DirectX::XMFLOAT3& GetAngularVelocity() const { return m_AngularVelocity; }
    const DirectX::XMFLOAT3X3& GetInvInertiaTensorWorld() const { return m_InvInertiaTensorWorld; }
    const Params& GetParams() const { return m_Params; }
    RigidbodyConstraints GetConstraints() const { return m_Constraints; }
    bool IsSleeping() const { return m_IsSleeping; }
    int GetGenerationId() const { return m_GenerationId; }
    float GetIgnoreCollisionTimer() const { return m_IgnoreCollisionTimer; }
    const Collider& GetLocalCollider() const { return m_LocalCollider; }
    bool IsGravityEnabled() const { return m_Params.useGravity; }
    void SetGravityEnabled(bool enabled) { m_Params.useGravity = enabled; }

    DirectX::XMFLOAT4X4 GetWorldMatrix(const DirectX::XMFLOAT3& scale) const;
    Collider GetWorldCollider() const;
    AABB GetTransformedAABB() const;

    // 定数
    static constexpr float MIN_MASS = 0.001f;
    static constexpr float MIN_INERTIA_FACTOR = 0.1f;
    static constexpr float MAX_DELTA_TIME = 0.02f;
    static constexpr float MAX_ROTATION_PER_FRAME = 0.5f;
    static constexpr float MAX_ARM_LENGTH = 2.0f;
    static constexpr float MAX_IMPULSE = 50.0f;
    static constexpr float SLEEP_TIME_THRESHOLD = 0.5f;
    static constexpr float SLEEP_ENERGY_BIAS = 0.5f;
    static constexpr float CORRECTION_PERCENT = 0.4f;
    static constexpr float CORRECTION_SLOP = 0.01f;
    static constexpr float VELOCITY_CUTOFF = 0.001f;

private:
    void UpdateInertiaTensor();
    void UpdateWorldMatrix();
    void CheckSleepState(float dt);
    DirectX::XMVECTOR UpdateRotation(DirectX::XMVECTOR qRot, DirectX::XMVECTOR vAngVel, float dt) const;
    void ApplyConstraintsToForces();
    void ApplyConstraintsToVelocities(DirectX::XMVECTOR& vVel, DirectX::XMVECTOR& vAngVel);

    static bool ApplyCollisionImpulse(RigidBody* rbA, RigidBody* rbB, const Hit& hit);
    static void ApplyPositionalCorrection(RigidBody* rbA, RigidBody* rbB, const Hit& hit, float totalInvMass);

    Params m_Params;

    DirectX::XMFLOAT3 m_Position;
    DirectX::XMFLOAT4 m_Rotation;
    DirectX::XMFLOAT3 m_Velocity;
    DirectX::XMFLOAT3 m_AngularVelocity;

    DirectX::XMFLOAT3 m_ForceAccum;
    DirectX::XMFLOAT3 m_TorqueAccum;

    Collider m_LocalCollider;
    AABB m_LocalAABB;
    DirectX::XMFLOAT4X4 m_WorldMatrix;
    DirectX::XMFLOAT3X3 m_InvInertiaTensorLocal;
    DirectX::XMFLOAT3X3 m_InvInertiaTensorWorld;

    RigidbodyConstraints m_Constraints;
    bool m_IsSleeping;
    float m_SleepTimer;
    bool m_HasGroundContact;  // 接地フラグ

    int m_GenerationId;
    float m_IgnoreCollisionTimer;
};

#endif // RIGID_BODY_H