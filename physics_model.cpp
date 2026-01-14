/****************************************
 * @file physics_model.cpp
 * @brief 物理挙動を持つモデルの実装
 * @detail ID管理などをRigidBodyへ委譲、衝突応答のリファクタリング
 ****************************************/

#include "physics_model.h"
#include "collision.h"
#include "collider_generator.h"
#include "map.h"
#include "player.h"
#include "stage.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

//======================================
// コンストラクタ
//======================================
PhysicsModel::PhysicsModel(MODEL* model, const XMFLOAT3& pos, const XMFLOAT3& vel, float mass, ColliderType colliderType)
    : m_pModel(model)
    , m_RigidBody()
    , m_Scale{ 1.0f, 1.0f, 1.0f }
    , m_rootVolume(0.0f)
    , m_lifeTimer(-1.0f)
    , m_isDead(false)
    , m_colliderType(colliderType)
{
    if (!m_pModel)
    {
        return;
    }

    ModelAddRef(m_pModel);

    // 最適なコライダーを生成
    Collider col = ColliderGenerator::GenerateBestFit(m_pModel, m_colliderType);

    // 生成直後の密着・スタック防止のためコライダーを縮小
    if (col.type == ColliderType::Box)
    {
        col.obb.extents.x = std::max(col.obb.extents.x - SKIN_WIDTH, 0.01f);
        col.obb.extents.y = std::max(col.obb.extents.y - SKIN_WIDTH, 0.01f);
        col.obb.extents.z = std::max(col.obb.extents.z - SKIN_WIDTH, 0.01f);
    }
    else if (col.type == ColliderType::Sphere)
    {
        col.sphere.radius = std::max(col.sphere.radius - SKIN_WIDTH, 0.01f);
    }

    // RigidBodyの初期化
    m_RigidBody.Initialize(pos, col, mass);
    m_RigidBody.SetVelocity(vel);
}

//======================================
// デストラクタ
//======================================
PhysicsModel::~PhysicsModel()
{
    if (m_pModel)
    {
        ModelRelease(m_pModel);
        m_pModel = nullptr;
    }
}

//======================================
// 更新処理
//======================================
void PhysicsModel::Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);

    // ライフタイマー処理と縮小演出
    if (m_lifeTimer > 0.0f)
    {
        m_lifeTimer -= dt;

        // 縮小演出（消滅の1秒前から開始）
        if (m_lifeTimer < SHRINK_START_TIME)
        {
            float ratio = std::max(m_lifeTimer / SHRINK_START_TIME, 0.0f);
            m_Scale = { ratio, ratio, ratio };
        }

        // 時間切れで死亡
        if (m_lifeTimer <= 0.0f)
        {
            m_isDead = true;
            return; // 死亡後は物理演算不要
        }
    }

    // モデルが無効な場合は処理しない
    if (!m_pModel)
    {
        return;
    }

    // 物理演算（積分）
    m_RigidBody.Integrate(dt);

    // 地形（MeshField）との衝突判定（重力が有効な場合のみ）
    if (m_RigidBody.IsGravityEnabled())
    {
        XMFLOAT3 currentPos = m_RigidBody.GetPosition();
        float terrainHeight = Stage_GetTerrainHeight(currentPos.x, currentPos.z);

        // コライダーの底面を考慮（中心オフセット + 半径/半高さ）
        float colliderBottomOffset = 0.0f;
        const Collider& col = m_RigidBody.GetLocalCollider();
        if (col.type == ColliderType::Box)
        {
            // OBBの場合：中心のYオフセット - 半高さ = 底面のオフセット
            colliderBottomOffset = col.obb.center.y - col.obb.extents.y;
        }
        else if (col.type == ColliderType::Sphere)
        {
            // Sphereの場合：中心のYオフセット - 半径 = 底面のオフセット
            colliderBottomOffset = col.sphere.center.y - col.sphere.radius;
        }

        // 実際の底面位置 = position.y + colliderBottomOffset
        float actualBottom = currentPos.y + colliderBottomOffset;

        // 地形より下にいる場合、地形に押し出す
        if (actualBottom < terrainHeight)
        {
            float correction = terrainHeight - actualBottom;
            currentPos.y += correction;
            m_RigidBody.SetPosition(currentPos);

            // 下向きの速度をリセットして軽く反発
            XMFLOAT3 vel = m_RigidBody.GetVelocity();
            if (vel.y < 0.0f)
            {
                vel.y *= -m_RigidBody.GetParams().restitution * 0.3f;
                // 小さな反発は無視
                if (fabsf(vel.y) < 0.5f) vel.y = 0.0f;
                m_RigidBody.SetVelocity(vel);
            }
        }
    }

    // 衝突判定の準備
    AABB worldAABB = m_RigidBody.GetTransformedAABB();

    // マップオブジェクト（壁など）との衝突判定
    const int objCount = Map_GetObjectCount();
    for (int i = 0; i < objCount; ++i)
    {
        const AABB& mapAABB = Map_GetObject(i)->Aabb;
        Hit hit = Collision_IsHitAABB(worldAABB, mapAABB);

        if (hit.isHit)
        {
            ApplyStaticCollisionResponse(hit);
        }
    }

    // プレイヤーとの衝突判定（プレイヤーは質量無限大の壁として扱う）
    OBB playerOBB = Player_GetWorldOBB(XMLoadFloat3(&Player_GetPosition()));
    Hit hitPlayer = Collision_IsHitOBBAABB(playerOBB, worldAABB);

    if (hitPlayer.isHit)
    {
        // 法線を反転（プレイヤー側からの押し出し方向に調整）
        XMVECTOR vNormal = XMLoadFloat3(&hitPlayer.normal);
        vNormal = XMVectorNegate(vNormal);
        XMStoreFloat3(&hitPlayer.normal, vNormal);

        ApplyStaticCollisionResponse(hitPlayer);
    }
}

//======================================
// 静的オブジェクトとの衝突応答処理
//======================================
void PhysicsModel::ApplyStaticCollisionResponse(const Hit& hit)
{
    // 物理パラメータの取得
    const RigidBody::Params params = m_RigidBody.GetParams();
    const float invMass = (params.mass > 0.0f) ? 1.0f / params.mass : 0.0f;

    // RigidBodyの状態を取得
    XMFLOAT3 fPos = m_RigidBody.GetPosition();
    XMFLOAT3 fVel = m_RigidBody.GetVelocity();
    XMFLOAT3 fAngVel = m_RigidBody.GetAngularVelocity();
    XMFLOAT3X3 fInvTensor = m_RigidBody.GetInvInertiaTensorWorld();

    // SIMDベクトルに変換
    XMVECTOR vPos = XMLoadFloat3(&fPos);
    XMVECTOR vVel = XMLoadFloat3(&fVel);
    XMVECTOR vAngVel = XMLoadFloat3(&fAngVel);
    XMMATRIX invInertiaWorld = XMLoadFloat3x3(&fInvTensor);

    XMVECTOR n = XMLoadFloat3(&hit.normal);
    XMVECTOR p = XMLoadFloat3(&hit.contactPoint);

    // 重心から接触点へのベクトル
    XMVECTOR r = XMVectorSubtract(p, vPos);

    // 接触点における速度: v_point = v + ω × r
    XMVECTOR vPoint = XMVectorAdd(vVel, XMVector3Cross(vAngVel, r));

    // 法線方向の速度成分
    float velAlongNormal = XMVectorGetX(XMVector3Dot(vPoint, n));

    // 既に離れようとしている場合は処理不要
    if (velAlongNormal > 0.0f)
    {
        return;
    }

    // ==========================================
    // A. 反発インパルスの計算
    // ==========================================
    float restitution = params.restitution;
    float j = -(1.0f + restitution) * velAlongNormal;

    XMVECTOR r_cross_n = XMVector3Cross(r, n);
    XMVECTOR termVec = XMVector3TransformNormal(r_cross_n, invInertiaWorld);
    float rotTerm = XMVectorGetX(XMVector3Dot(termVec, r_cross_n));

    j /= (invMass + rotTerm);

    XMVECTOR impulse = XMVectorScale(n, j);
    XMFLOAT3 fImpulse;
    XMStoreFloat3(&fImpulse, impulse);
    XMFLOAT3 fContact;
    XMStoreFloat3(&fContact, p);

    m_RigidBody.ApplyImpulseAtPoint(fImpulse, fContact);

    // ==========================================
    // B. 摩擦インパルスの計算
    // ==========================================
    // 反発後の速度で再計算
    XMFLOAT3 rb_vel = m_RigidBody.GetVelocity();
    XMFLOAT3 rb_angularVel = m_RigidBody.GetAngularVelocity();
    vPoint = XMVectorAdd(XMLoadFloat3(&rb_vel),
        XMVector3Cross(XMLoadFloat3(&rb_angularVel), r));

    // 接線方向ベクトルの計算
    XMVECTOR tangent = XMVectorSubtract(vPoint,
        XMVectorScale(n, XMVectorGetX(XMVector3Dot(vPoint, n))));
    float tangentLenSq = XMVectorGetX(XMVector3LengthSq(tangent));

    if (tangentLenSq > 1e-6f)
    {
        tangent = XMVector3Normalize(tangent);
        float jt = -XMVectorGetX(XMVector3Dot(vPoint, tangent));

        XMVECTOR r_cross_t = XMVector3Cross(r, tangent);
        XMVECTOR termVecT = XMVector3TransformNormal(r_cross_t, invInertiaWorld);
        float rotTermT = XMVectorGetX(XMVector3Dot(termVecT, r_cross_t));

        jt /= (invMass + rotTermT);

        // クーロン摩擦モデル
        float mu = params.friction;
        if (std::abs(jt) > j * mu)
        {
            jt = (jt > 0.0f ? 1.0f : -1.0f) * j * mu;
        }

        XMVECTOR frictionImpulse = XMVectorScale(tangent, jt);
        XMFLOAT3 fFriction;
        XMStoreFloat3(&fFriction, frictionImpulse);

        m_RigidBody.ApplyImpulseAtPoint(fFriction, fContact);
    }

    // ==========================================
    // C. 位置補正（貫通防止）
    // ==========================================
    float correctionDepth = std::max(hit.depth - CORRECTION_SLOP, 0.0f);

    if (correctionDepth > 0.0f)
    {
        XMVECTOR correction = XMVectorScale(n, correctionDepth * CORRECTION_PERCENT);
        XMFLOAT3 fCurrentPos = m_RigidBody.GetPosition();
        XMVECTOR vCurrentPos = XMLoadFloat3(&fCurrentPos);

        XMFLOAT3 newPos;
        XMStoreFloat3(&newPos, XMVectorAdd(vCurrentPos, correction));
        m_RigidBody.SetPosition(newPos);
    }
}

//======================================
// 描画処理
//======================================
void PhysicsModel::Draw()
{
    if (m_pModel)
    {
        XMFLOAT4X4 matWorld = m_RigidBody.GetWorldMatrix(m_Scale);
        XMMATRIX world = XMLoadFloat4x4(&matWorld);
        ModelDraw(m_pModel, world);
    }
}