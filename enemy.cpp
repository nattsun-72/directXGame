/****************************************
 * @file enemy.cpp
 * @brief 敵キャラクタ管理システムの実装
 * @author Natsume Shidara
 * @date 2025/11/26
 * @update 2026/01/13 - EnemyFlying統合
 ****************************************/

#include "enemy.h"
#include "enemy_ground.h"
#include "enemy_flying.h"
#include "prop_manager.h"
#include "slicer.h"
#include "slice_task_manager.h"
#include "collision.h"
#include "score.h"
#include "combo.h"
#include "player.h"

#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cmath>

using namespace DirectX;

//--------------------------------------
// 定数
//--------------------------------------
namespace
{
    // 切断設定
    constexpr float MIN_VOLUME = 0.0001f;

    // 消滅タイマー（0.25秒待機 + 1秒縮小 = 1.25秒）
    constexpr float DEBRIS_LIFETIME = 1.25f;

    // スコア設定
    constexpr int SCORE_PER_SLICE = 100;
    constexpr int SCORE_KILL_BONUS = 500;
}

//--------------------------------------
// 管理用変数
//--------------------------------------
static std::vector<Enemy*> g_Enemies;

// 切断処理中の敵情報
struct PendingSliceInfo
{
    Enemy* pEnemy;
    DirectX::XMFLOAT3 planeNormal;
    ENEMY_TYPE enemyType;
};
static std::unordered_map<int, PendingSliceInfo> g_PendingSliceEnemies;

//======================================
// ヘルパー関数
//======================================
namespace
{
    /**
     * @brief MeshDataからAABBベースの体積を推定
     */
    float EstimateVolumeFromMeshes(const std::vector<MeshData>& meshes)
    {
        if (meshes.empty()) return 0.0f;

        XMFLOAT3 minPos = { FLT_MAX, FLT_MAX, FLT_MAX };
        XMFLOAT3 maxPos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

        for (const auto& mesh : meshes)
        {
            for (const auto& vert : mesh.vertices)
            {
                minPos.x = std::min(minPos.x, vert.position.x);
                minPos.y = std::min(minPos.y, vert.position.y);
                minPos.z = std::min(minPos.z, vert.position.z);
                maxPos.x = std::max(maxPos.x, vert.position.x);
                maxPos.y = std::max(maxPos.y, vert.position.y);
                maxPos.z = std::max(maxPos.z, vert.position.z);
            }
        }

        float sizeX = maxPos.x - minPos.x;
        float sizeY = maxPos.y - minPos.y;
        float sizeZ = maxPos.z - minPos.z;

        return sizeX * sizeY * sizeZ;
    }

    /**
     * @brief 切断破片を処理（敵として継続 or 残骸化）
     */
    void ProcessSlicedPiece(
        const std::vector<MeshData>& meshes,
        MODEL* originalModel,
        const SliceResult& result,
        const XMFLOAT3& planeNormal,
        float rootVolume,
        bool isFrontSide,
        ENEMY_TYPE enemyType)
    {
        if (meshes.empty()) return;

        // 体積を推定
        float pieceVolume = EstimateVolumeFromMeshes(meshes);
        float volumeRatio = (rootVolume > MIN_VOLUME) ? (pieceVolume / rootVolume) : 0.0f;

        // 分離パラメータを計算
        XMVECTOR vPos = XMLoadFloat3(&result.originalPosition);
        XMVECTOR vVel = XMLoadFloat3(&result.originalVelocity);
        XMVECTOR vNormal = XMLoadFloat3(&planeNormal);

        // 敵タイプに応じた分離パラメータ
        float separationSpeed = EnemyGroundConfig::SLICE_SEPARATION_SPEED;
        float offsetDistance = EnemyGroundConfig::SLICE_OFFSET_DISTANCE;
        float deathThreshold = EnemyGroundConfig::DEATH_VOLUME_THRESHOLD;

        if (enemyType == ENEMY_TYPE_FLYING)
        {
            separationSpeed = EnemyFlyingConfig::SLICE_SEPARATION_SPEED;
            offsetDistance = EnemyFlyingConfig::SLICE_OFFSET_DISTANCE;
            deathThreshold = EnemyFlyingConfig::DEATH_VOLUME_THRESHOLD;
        }

        float sign = isFrontSide ? 1.0f : -1.0f;
        vPos = vPos + vNormal * (offsetDistance * sign);
        vVel = vVel + vNormal * (separationSpeed * sign);

        XMFLOAT3 newPosition, newVelocity;
        XMStoreFloat3(&newPosition, vPos);
        XMStoreFloat3(&newVelocity, vVel);

        // 体積比率で振り分け（50%超で敵として継続）
        if (volumeRatio > deathThreshold)
        {
            // 50%超 → 敵として継続
            if (enemyType == ENEMY_TYPE_GROUND)
            {
                SlicedEnemyParams params;
                params.meshes = meshes;
                params.originalModel = originalModel;
                params.position = newPosition;
                params.velocity = newVelocity;
                params.planeNormal = planeNormal;
                params.mass = result.originalMass * volumeRatio;
                params.rootVolume = rootVolume;
                params.colliderType = result.colliderType;
                params.isFrontSide = isFrontSide;

                EnemyGround* pNewEnemy = EnemyGround_CreateFromSlice(params);
                if (pNewEnemy)
                {
                    g_Enemies.push_back(pNewEnemy);
                }
            }
            else if (enemyType == ENEMY_TYPE_FLYING)
            {
                SlicedFlyingEnemyParams params;
                params.meshes = meshes;
                params.originalModel = originalModel;
                params.position = newPosition;
                params.velocity = newVelocity;
                params.planeNormal = planeNormal;
                params.mass = result.originalMass * volumeRatio;
                params.rootVolume = rootVolume;
                params.colliderType = result.colliderType;
                params.isFrontSide = isFrontSide;

                EnemyFlying* pNewEnemy = EnemyFlying_CreateFromSlice(params);
                if (pNewEnemy)
                {
                    g_Enemies.push_back(pNewEnemy);
                }
            }
        }
        else
        {
            // 50%以下 → PropManagerへ（残骸化）
            SlicedPieceParams params;
            params.meshes = meshes;
            params.originalModel = originalModel;
            params.position = newPosition;
            params.velocity = newVelocity;
            params.planeNormal = planeNormal;
            params.mass = result.originalMass * volumeRatio;
            params.rootVolume = rootVolume;
            params.lifeTime = DEBRIS_LIFETIME;
            params.colliderType = result.colliderType;
            params.isFrontSide = isFrontSide;

            PropManager_AddSlicedPiece(params);

            // 撃破ボーナス（コンボ倍率適用）
            int comboCount = Combo_GetCount();
            float multiplier = 1.0f + comboCount * 0.1f;
            multiplier = std::min(multiplier, 5.0f);
            int killBonus = static_cast<int>(SCORE_KILL_BONUS * multiplier);
            Score_AddScore(killBonus);
        }
    }
}

//======================================
// Enemy クラス実装
//======================================

Enemy::~Enemy()
{
    if (m_pState)
    {
        delete m_pState;
        m_pState = nullptr;
    }
    if (m_pNextState)
    {
        delete m_pNextState;
        m_pNextState = nullptr;
    }
}

void Enemy::Update(float dt)
{
    if (m_pState)
    {
        m_pState->Update(dt);
    }
}

void Enemy::Draw() const
{
    if (m_pState)
    {
        m_pState->Draw();
    }
}

void Enemy::DrawShadow() const
{
    if (m_pState)
    {
        m_pState->DrawShadow();
    }
}

void Enemy::DrawDebug() const
{
    if (m_pState)
    {
        m_pState->DrawDebug();
    }
}

void Enemy::UpdateState()
{
    if (m_pNextState)
    {
        if (m_pState)
        {
            m_pState->Exit();
            delete m_pState;
        }

        m_pState = m_pNextState;
        m_pNextState = nullptr;

        if (m_pState)
        {
            m_pState->Enter();
        }
    }
}

void Enemy::ChangeState(State* pNext)
{
    if (m_pNextState)
    {
        delete m_pNextState;
    }
    m_pNextState = pNext;
}

void Enemy::TakeDamage(float volumeLost)
{
    m_VolumeRatio -= volumeLost;
    m_VolumeRatio = std::max(0.0f, m_VolumeRatio);

    if (m_VolumeRatio <= EnemyGroundConfig::DEATH_VOLUME_THRESHOLD)
    {
        m_IsDestroyed = true;
    }
}

//======================================
// グローバル管理関数
//======================================

void Enemy_Initialize()
{
    g_Enemies.clear();
    g_Enemies.reserve(64);

    EnemyGround_InitializeShared();
    EnemyFlying_InitializeShared();
}

void Enemy_Finalize()
{
    for (Enemy* pEnemy : g_Enemies)
    {
        delete pEnemy;
    }
    g_Enemies.clear();

    for (auto& pair : g_PendingSliceEnemies)
    {
        delete pair.second.pEnemy;
    }
    g_PendingSliceEnemies.clear();

    EnemyGround_FinalizeShared();
    EnemyFlying_FinalizeShared();
}

void Enemy_Update(float dt)
{
    // 1. 予約された状態遷移を確定
    for (Enemy* pEnemy : g_Enemies)
    {
        pEnemy->UpdateState();
    }

    // 2. 行動更新 & 死亡判定
    for (int i = static_cast<int>(g_Enemies.size()) - 1; i >= 0; i--)
    {
        if (g_Enemies[i]->IsDestroy())
        {
            delete g_Enemies[i];
            g_Enemies.erase(g_Enemies.begin() + i);
        }
        else
        {
            g_Enemies[i]->Update(dt);
        }
    }
}

void Enemy_Draw()
{
    for (const Enemy* pEnemy : g_Enemies)
    {
        pEnemy->Draw();
    }
}

void Enemy_DrawShadow()
{
    for (const Enemy* pEnemy : g_Enemies)
    {
        pEnemy->DrawShadow();
    }
}

void Enemy_DrawDebug()
{
    for (const Enemy* pEnemy : g_Enemies)
    {
        pEnemy->DrawDebug();
    }
}

void Enemy_Create(ENEMY_TYPE type, const XMFLOAT3& position)
{
    Enemy* pEnemy = nullptr;

    switch (type)
    {
    case ENEMY_TYPE_GROUND:
        pEnemy = new EnemyGround(position);
        break;

    case ENEMY_TYPE_FLYING:
        pEnemy = new EnemyFlying(position);
        break;

    default:
        break;
    }

    if (pEnemy)
    {
        g_Enemies.push_back(pEnemy);
    }
}

void Enemy_Clear()
{
    for (Enemy* pEnemy : g_Enemies)
    {
        delete pEnemy;
    }
    g_Enemies.clear();
}

int Enemy_GetCount()
{
    return static_cast<int>(g_Enemies.size());
}

int Enemy_GetAliveCount()
{
    int count = 0;
    for (const Enemy* pEnemy : g_Enemies)
    {
        if (!pEnemy->IsDestroy()) count++;
    }
    return count;
}

Enemy* Enemy_GetEnemy(int index)
{
    if (index < 0 || index >= static_cast<int>(g_Enemies.size()))
        return nullptr;
    return g_Enemies[index];
}

//======================================
// 切断処理
//======================================

void Enemy_TrySlice(const Ray& ray, const XMFLOAT3& planeNormal)
{
    for (auto it = g_Enemies.begin(); it != g_Enemies.end();)
    {
        Enemy* pEnemy = *it;

        // PhysicsModelと敵タイプを取得
        PhysicsModel* pPhysics = nullptr;
        ENEMY_TYPE enemyType = ENEMY_TYPE_GROUND;

        EnemyGround* pGround = dynamic_cast<EnemyGround*>(pEnemy);
        EnemyFlying* pFlying = dynamic_cast<EnemyFlying*>(pEnemy);

        if (pGround && pGround->GetPhysicsModel())
        {
            pPhysics = pGround->GetPhysicsModel();
            enemyType = ENEMY_TYPE_GROUND;
        }
        else if (pFlying && pFlying->GetPhysicsModel())
        {
            pPhysics = pFlying->GetPhysicsModel();
            enemyType = ENEMY_TYPE_FLYING;
        }

        if (!pPhysics)
        {
            ++it;
            continue;
        }

        MODEL* pModel = pPhysics->GetModel();
        if (!pModel)
        {
            ++it;
            continue;
        }

        // AABBでの粗い判定
        XMFLOAT3 pos = pPhysics->GetPosition();
        AABB worldAABB = Model_GetAABB(pModel, pos);

        float hitDist = 0.0f;
        if (!Collision_IntersectRayAABB(ray, worldAABB, &hitDist))
        {
            ++it;
            continue;
        }

        // ヒット位置
        XMFLOAT3 rayOrigin = ray.GetOrigin();
        XMFLOAT3 rayDir = ray.GetDirection();
        XMVECTOR vHitPos = XMLoadFloat3(&rayOrigin) + XMLoadFloat3(&rayDir) * hitDist;
        XMFLOAT3 hitPos;
        XMStoreFloat3(&hitPos, vHitPos);

        // 切断リクエストを送信
        RigidBody* rb = pPhysics->GetRigidBody();

        SliceRequest request;
        request.targetModel = pModel;
        request.worldMatrix = rb->GetWorldMatrix(XMFLOAT3(1.0f, 1.0f, 1.0f));
        request.planePoint = hitPos;
        request.planeNormal = planeNormal;
        request.originalPosition = rb->GetPosition();
        request.originalVelocity = rb->GetVelocity();
        request.originalMass = rb->GetParams().mass;
        request.rootVolume = pPhysics->GetRootVolume();
        request.colliderType = pPhysics->GetColliderType();

        int requestId = SliceTaskManager::EnqueueSlice(request);

        // 切断待ちリストに移動（敵タイプも保存）
        PendingSliceInfo info;
        info.pEnemy = pEnemy;
        info.planeNormal = planeNormal;
        info.enemyType = enemyType;
        g_PendingSliceEnemies[requestId] = info;
        it = g_Enemies.erase(it);
    }
}

bool Enemy_ProcessSliceResults()
{
    SliceResult result;
    bool processed = false;

    while (SliceTaskManager::TryGetCompletedResult(result))
    {
        // 敵の切断結果かどうか確認
        auto it = g_PendingSliceEnemies.find(result.requestId);
        if (it == g_PendingSliceEnemies.end())
        {
            // 敵ではない（PropManagerの処理に任せる）
            continue;
        }

        processed = true;
        PendingSliceInfo& info = it->second;
        Enemy* pOriginalEnemy = info.pEnemy;
        XMFLOAT3 planeNormal = info.planeNormal;
        ENEMY_TYPE enemyType = info.enemyType;
        g_PendingSliceEnemies.erase(it);

        if (!result.success)
        {
            // 切断失敗 - 元に戻す
            g_Enemies.push_back(pOriginalEnemy);
            ModelRelease(result.originalModel);
            continue;
        }

        // 切断成功 - 体積に応じて振り分け
        float rootVolume = result.rootVolume;

        // コンボ追加
        Combo_Add(1);

        // スコア加算（コンボボーナス込み）
        int bonusScore = Combo_GetBonusScore();
        Score_AddScore(bonusScore);

        // AirDash中に切断成功した場合、AirDashを回復
        PlayerState playerState = Player_GetState();
        if (playerState == PlayerState::AirDash || playerState == PlayerState::AirDashCharge)
        {
            Player_RecoverAirDash(1);
        }

        // Front側の処理（敵タイプを渡す）
        ProcessSlicedPiece(result.frontMeshes, result.originalModel, result, planeNormal, rootVolume, true, enemyType);

        // Back側の処理（敵タイプを渡す）
        ProcessSlicedPiece(result.backMeshes, result.originalModel, result, planeNormal, rootVolume, false, enemyType);

        // 元の敵は削除
        delete pOriginalEnemy;
        ModelRelease(result.originalModel);
    }

    return processed;
}