/****************************************
 * @file enemy_ground.h
 * @brief 地上型敵（遠距離攻撃・追尾型）
 * @detail
 *   - PhysicsModelをコンポジションで内包（物理+切断）
 *   - 最適距離を維持しようとする
 *   - 遠い → 接近 / 最適距離 → 射撃 / 近い → 後退
 * @author Natsume Shidara
 * @date 2026/01/10
 ****************************************/

#ifndef ENEMY_GROUND_H
#define ENEMY_GROUND_H

#include "enemy.h"
#include "physics_model.h"
#include <DirectXMath.h>

 //======================================
 // 調整用定数（外部公開）
 //======================================
namespace EnemyGroundConfig
{
    // 射撃パラメータ
    constexpr float ATTACK_DURATION = 5.0f;
    constexpr float RELOAD_DURATION = 0.5f;
    constexpr float FIRE_INTERVAL = 0.2f;

    // 距離判定
    constexpr float RANGE_NEAR = 5.0f;
    constexpr float RANGE_FAR = 15.0f;

    // 移動速度
    constexpr float APPROACH_SPEED = 8.0f;
    constexpr float RETREAT_SPEED = 4.0f;

    // 壁判定
    constexpr float WALL_CHECK_DISTANCE = 2.0f;

    // 物理パラメータ
    constexpr float MASS = 50.0f;

    // 切断パラメータ
    constexpr float DEATH_VOLUME_THRESHOLD = 0.8f;    // 50%以下で死亡（二等分で死亡）

    // 分離パラメータ（めり込み防止）
    constexpr float SLICE_SEPARATION_SPEED = 5.0f;    // 分離速度
    constexpr float SLICE_OFFSET_DISTANCE = 0.15f;    // 位置オフセット
    constexpr float SLICE_TORQUE_STRENGTH = 8.0f;     // 回転トルク
    constexpr float SLICE_IMMUNITY_TIME = 0.3f;       // 衝突無視時間

    // モデルパス
    constexpr const char* MODEL_PATH = "assets/fbx/BBOXX.fbx";
    constexpr const wchar_t* TEXTURE_PATH = L"assets/fbx/enemy_sl.png";
    constexpr float MODEL_SCALE = 2.0f;
}

// 前方宣言
struct SlicedEnemyParams;

//======================================
// 地上型敵クラス
//======================================
class EnemyGround : public Enemy
{
    // 切断結果からの生成関数にprivateメンバへのアクセスを許可
    friend EnemyGround* EnemyGround_CreateFromSlice(const SlicedEnemyParams& params);

public:
    //----------------------------------
    // 状態クラス（前方宣言）
    //----------------------------------
    class StateIdle;
    class StateApproach;
    class StateAttack;
    class StateReload;
    class StateRetreat;

private:
    //----------------------------------
    // 物理モデル（コンポジション）
    //----------------------------------
    PhysicsModel* m_pPhysics = nullptr;

    //----------------------------------
    // AI用
    //----------------------------------
    float m_DistanceToPlayer = 0.0f;
    float m_FireTimer = 0.0f;

public:
    //----------------------------------
    // コンストラクタ/デストラクタ
    //----------------------------------
    EnemyGround(const DirectX::XMFLOAT3& position);
    ~EnemyGround();

    // コピー禁止
    EnemyGround(const EnemyGround&) = delete;
    EnemyGround& operator=(const EnemyGround&) = delete;

    //----------------------------------
    // オーバーライド
    //----------------------------------
    bool IsDestroy() const override;

    //----------------------------------
    // PhysicsModelアクセサ
    //----------------------------------
    PhysicsModel* GetPhysicsModel() { return m_pPhysics; }
    const PhysicsModel* GetPhysicsModel() const { return m_pPhysics; }

    // 位置の取得・設定（PhysicsModel経由）
    DirectX::XMFLOAT3 GetPosition() const;
    void SetPosition(const DirectX::XMFLOAT3& pos);

    //----------------------------------
    // AI用アクセサ
    //----------------------------------
    float GetDistanceToPlayer() const { return m_DistanceToPlayer; }
    float& GetFireTimer() { return m_FireTimer; }

    //----------------------------------
    // AI処理
    //----------------------------------
    void UpdateDistanceAndFacing();
    void FireBullet();
    DirectX::XMFLOAT3 CalculateRetreatDirection();
    bool CheckWall(const DirectX::XMFLOAT3& direction, float distance);

    //----------------------------------
    // 移動処理
    //----------------------------------
    void Move(const DirectX::XMFLOAT3& direction, float speed);
    void StopMovement();

    //----------------------------------
    // 切断判定用
    //----------------------------------
    void CheckVolumeAndDeath();
};

//======================================
// 状態クラス定義
//======================================

class EnemyGround::StateIdle : public Enemy::State
{
public:
    StateIdle(EnemyGround* pOwner) : State(pOwner) {}
    void Update(float dt) override;
    void Draw() const override;
    void DrawDebug() const override;
};

class EnemyGround::StateApproach : public Enemy::State
{
public:
    StateApproach(EnemyGround* pOwner) : State(pOwner) {}
    void Update(float dt) override;
    void Draw() const override;
    void DrawDebug() const override;
};

class EnemyGround::StateAttack : public Enemy::State
{
private:
    float m_StateTimer = 0.0f;
public:
    StateAttack(EnemyGround* pOwner) : State(pOwner) {}
    void Enter() override;
    void Update(float dt) override;
    void Draw() const override;
    void DrawDebug() const override;
};

class EnemyGround::StateReload : public Enemy::State
{
private:
    float m_StateTimer = 0.0f;
public:
    StateReload(EnemyGround* pOwner) : State(pOwner) {}
    void Update(float dt) override;
    void Draw() const override;
    void DrawDebug() const override;
};

class EnemyGround::StateRetreat : public Enemy::State
{
public:
    StateRetreat(EnemyGround* pOwner) : State(pOwner) {}
    void Update(float dt) override;
    void Draw() const override;
    void DrawDebug() const override;
};

//======================================
// 共有リソース管理
//======================================
void EnemyGround_InitializeShared();
void EnemyGround_FinalizeShared();

//======================================
// 切断結果からの生成（内部用）
//======================================
struct SlicedEnemyParams
{
    std::vector<MeshData> meshes;
    MODEL* originalModel;
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 velocity;
    DirectX::XMFLOAT3 planeNormal;
    float mass;
    float rootVolume;
    ColliderType colliderType;
    bool isFrontSide;
};

/**
 * @brief 切断結果からEnemyGroundを生成
 * @return 生成されたEnemyGround（失敗時はnullptr）
 */
EnemyGround* EnemyGround_CreateFromSlice(const SlicedEnemyParams& params);

#endif // ENEMY_GROUND_H