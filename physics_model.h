/****************************************
 * @file physics_model.h
 * @brief 物理挙動を持つモデルのヘッダー
 * @detail RigidBodyへの委譲、自動消滅機能を含む
 * @author NatsumeShidara
 ****************************************/

#ifndef PHYSICS_MODEL_H
#define PHYSICS_MODEL_H

#include <DirectXMath.h>
#include "model.h"
#include "rigid_body.h"

class PhysicsModel
{
public:
    // コンストラクタ・デストラクタ
    PhysicsModel(MODEL* model, const DirectX::XMFLOAT3& pos,
        const DirectX::XMFLOAT3& vel, float mass = 10.0f, ColliderType colliderType = ColliderType::Box);
    ~PhysicsModel();

    // コピー・ムーブ禁止（リソース管理の安全性のため）
    PhysicsModel(const PhysicsModel&) = delete;
    PhysicsModel& operator=(const PhysicsModel&) = delete;
    PhysicsModel(PhysicsModel&&) = delete;
    PhysicsModel& operator=(PhysicsModel&&) = delete;

    // 更新・描画
    void Update(double elapsed_time);
    void Draw();

    // RigidBodyへの直接アクセス
    RigidBody* GetRigidBody() { return &m_RigidBody; }
    const RigidBody* GetRigidBody() const { return &m_RigidBody; }

    // 基本プロパティ（RigidBodyへの委譲）
    DirectX::XMFLOAT3 GetPosition() const { return m_RigidBody.GetPosition(); }
    void SetPosition(const DirectX::XMFLOAT3& pos) { m_RigidBody.SetPosition(pos); }

    DirectX::XMFLOAT3 GetVelocity() const { return m_RigidBody.GetVelocity(); }
    void SetVelocity(const DirectX::XMFLOAT3& vel) { m_RigidBody.SetVelocity(vel); }

    // モデルアクセス
    MODEL* GetModel() const { return m_pModel; }

    // スケール管理
    DirectX::XMFLOAT3 GetScale() const { return m_Scale; }
    void SetScale(const DirectX::XMFLOAT3& scale) { m_Scale = scale; }

    // 世代ID管理（RigidBodyへの委譲）
    void SetSliceGroupId(int id) { m_RigidBody.SetGenerationId(id); }
    int GetSliceGroupId() const { return m_RigidBody.GetGenerationId(); }

    // 衝突無視タイマー（RigidBodyへの委譲）
    void SetIgnoreCollisionTimer(float time) { m_RigidBody.SetIgnoreCollisionTimer(time); }
    float GetIgnoreCollisionTimer() const { return m_RigidBody.GetIgnoreCollisionTimer(); }

    // 自動消滅機能
    void SetRootVolume(float vol) { m_rootVolume = vol; }
    float GetRootVolume() const { return m_rootVolume; }

    void SetAutoDestroyTimer(float time) { m_lifeTimer = time; }
    float GetAutoDestroyTimer() const { return m_lifeTimer; }

    void SetColliderType(ColliderType colliderType) { m_colliderType = colliderType; }
    ColliderType GetColliderType() const { return m_colliderType; }

    bool IsDead() const { return m_isDead; }

private:
    // 衝突応答処理（内部ヘルパー）
    void ApplyStaticCollisionResponse(const Hit& hit);

    // メンバ変数
    MODEL* m_pModel;                    // モデルへのポインタ
    RigidBody m_RigidBody;              // 物理演算本体
    DirectX::XMFLOAT3 m_Scale;          // スケール

    // 自動消滅関連
    float m_rootVolume;                 // 祖先の体積（基準値）
    float m_lifeTimer;                  // 自動消滅までの残り時間（負=無効）
    bool m_isDead;                      // 消滅フラグ

    ColliderType m_colliderType;

    // 定数
    static constexpr float SKIN_WIDTH = -0.01f;        // コライダーのスキン幅
    static constexpr float SHRINK_START_TIME = 1.0f;   // 縮小開始時間
    static constexpr float CORRECTION_PERCENT = 0.2f;  // 位置補正の割合
    static constexpr float CORRECTION_SLOP = 0.01f;    // 位置補正のスロップ
};

#endif // PHYSICS_MODEL_H