/****************************************
 * @file enemy_flying.h
 * @brief 飛行型敵（突撃・撤退型）
 * @detail
 *   - 空中をホバリングしながら移動
 *   - プレイヤーに向かって突撃攻撃
 *   - 攻撃後は上空へ撤退してクールダウン
 * @author NatsumeShidara
 * @date 2026/01/13
 ****************************************/

#ifndef ENEMY_FLYING_H
#define ENEMY_FLYING_H

#include "enemy.h"
#include "physics_model.h"
#include <DirectXMath.h>

 //======================================
 // 調整用定数
 //======================================
namespace EnemyFlyingConfig
{
    // 飛行パラメータ
    constexpr float HOVER_HEIGHT_MIN = 8.0f;       // 最低ホバリング高度
    constexpr float HOVER_HEIGHT_MAX = 15.0f;      // 最高ホバリング高度
    constexpr float HOVER_AMPLITUDE = 1.0f;        // ホバリング上下幅
    constexpr float HOVER_SPEED = 2.0f;            // ホバリング上下速度
    constexpr float HOVER_MOVE_SPEED = 3.0f;       // ホバリング中の水平移動速度

    // 突撃パラメータ
    constexpr float CHARGE_SPEED = 24000.0f;          // 突撃速度
    constexpr float CHARGE_ACCELERATION = 1000.0f;  // 突撃加速度
    constexpr float CHARGE_DISTANCE = 3.0f;        // 突撃終了距離（プレイヤーとの距離）
    constexpr float CHARGE_WINDUP_TIME = 0.25f;     // 突撃前の溜め時間
    constexpr float CHARGE_MAX_DURATION = 3.0f;    // 突撃最大時間（タイムアウト）

    // 撤退パラメータ
    constexpr float RETREAT_SPEED = 12.0f;         // 撤退速度
    constexpr float RETREAT_HEIGHT = 12.0f;        // 撤退時の目標高度
    constexpr float RETREAT_DISTANCE = 15.0f;      // 撤退時のプレイヤーからの距離
    constexpr float RETREAT_DURATION = 1.5f;       // 撤退時間

    // クールダウン
    constexpr float COOLDOWN_DURATION = 2.0f;      // 次の突撃までの待機時間

    // 旋回パラメータ
    constexpr float CIRCLE_SPEED = 2.0f;           // 旋回速度
    constexpr float CIRCLE_RADIUS = 8.0f;          // 旋回半径

    // 距離判定
    constexpr float ATTACK_RANGE = 20.0f;          // 攻撃開始距離

    // 物理パラメータ
    constexpr float MASS = 30.0f;
    constexpr float DRAG = 5.0f;                   // 空気抵抗

    // 切断パラメータ
    constexpr float DEATH_VOLUME_THRESHOLD = 0.95f;
    constexpr float SLICE_SEPARATION_SPEED = 5.0f;
    constexpr float SLICE_OFFSET_DISTANCE = 0.15f;
    constexpr float SLICE_TORQUE_STRENGTH = 10.0f;
    constexpr float SLICE_IMMUNITY_TIME = 0.1f;

    // モデルパス（仮：BOXを使用）
    constexpr const char* MODEL_PATH = "assets/fbx/option_booster/option_booster2.fbx";
    constexpr const wchar_t* TEXTURE_PATH = L"assets/fbx/option_booster/drone_danmen.png";
    constexpr float MODEL_SCALE = 1.8f;
}

// 前方宣言
struct SlicedFlyingEnemyParams;

//======================================
// 飛行型敵クラス
//======================================
class EnemyFlying : public Enemy
{
    friend EnemyFlying* EnemyFlying_CreateFromSlice(const SlicedFlyingEnemyParams& params);

public:
    //----------------------------------
    // 状態クラス（前方宣言）
    //----------------------------------
    class StateHover;      // ホバリング（待機・旋回）
    class StateWindup;     // 突撃前の溜め
    class StateCharge;     // 突撃
    class StateRetreat;    // 撤退
    class StateCooldown;   // クールダウン

private:
    //----------------------------------
    // 物理モデル
    //----------------------------------
    PhysicsModel* m_pPhysics = nullptr;

    //----------------------------------
    // AI用
    //----------------------------------
    float m_DistanceToPlayer = 0.0f;
    float m_HoverTimer = 0.0f;           // ホバリング用タイマー
    float m_CircleAngle = 0.0f;          // 旋回角度
    DirectX::XMFLOAT3 m_ChargeTarget;    // 突撃目標位置
    DirectX::XMFLOAT3 m_ChargeDirection; // 突撃方向

public:
    //----------------------------------
    // コンストラクタ/デストラクタ
    //----------------------------------
    EnemyFlying(const DirectX::XMFLOAT3& position);
    ~EnemyFlying();

    // コピー禁止
    EnemyFlying(const EnemyFlying&) = delete;
    EnemyFlying& operator=(const EnemyFlying&) = delete;

    //----------------------------------
    // オーバーライド
    //----------------------------------
    bool IsDestroy() const override;

    //----------------------------------
    // PhysicsModelアクセサ
    //----------------------------------
    PhysicsModel* GetPhysicsModel() { return m_pPhysics; }
    const PhysicsModel* GetPhysicsModel() const { return m_pPhysics; }

    DirectX::XMFLOAT3 GetPosition() const;
    void SetPosition(const DirectX::XMFLOAT3& pos);
    DirectX::XMFLOAT3 GetVelocity() const;
    void SetVelocity(const DirectX::XMFLOAT3& vel);

    //----------------------------------
    // AI用アクセサ
    //----------------------------------
    float GetDistanceToPlayer() const { return m_DistanceToPlayer; }
    float& GetHoverTimer() { return m_HoverTimer; }
    float& GetCircleAngle() { return m_CircleAngle; }
    DirectX::XMFLOAT3& GetChargeTarget() { return m_ChargeTarget; }
    DirectX::XMFLOAT3& GetChargeDirection() { return m_ChargeDirection; }

    //----------------------------------
    // AI処理
    //----------------------------------
    void UpdateDistanceAndFacing();
    void ApplyHoverForce(float dt);
    void MoveToward(const DirectX::XMFLOAT3& target, float speed, float dt);
    void CircleAroundPlayer(float dt);

    //----------------------------------
    // 突撃関連
    //----------------------------------
    void PrepareCharge();
    void ExecuteCharge(float dt);
    bool HasReachedChargeTarget() const;

    void ClampToTerrain();
};

//======================================
// 状態クラス定義
//======================================

class EnemyFlying::StateHover : public Enemy::State
{
private:
    float m_StateTimer = 0.0f;
public:
    StateHover(EnemyFlying* pOwner) : State(pOwner) {}
    void Enter() override;
    void Update(float dt) override;
    void Draw() const override;
    void DrawShadow() const override;
    void DrawDebug() const override;
};

class EnemyFlying::StateWindup : public Enemy::State
{
private:
    float m_StateTimer = 0.0f;
public:
    StateWindup(EnemyFlying* pOwner) : State(pOwner) {}
    void Enter() override;
    void Update(float dt) override;
    void Draw() const override;
    void DrawShadow() const override;
    void DrawDebug() const override;
};

class EnemyFlying::StateCharge : public Enemy::State
{
private:
    float m_StateTimer = 0.0f;
public:
    StateCharge(EnemyFlying* pOwner) : State(pOwner) {}
    void Enter() override;
    void Update(float dt) override;
    void Draw() const override;
    void DrawShadow() const override;
    void DrawDebug() const override;
};

class EnemyFlying::StateRetreat : public Enemy::State
{
private:
    float m_StateTimer = 0.0f;
    DirectX::XMFLOAT3 m_RetreatTarget;
public:
    StateRetreat(EnemyFlying* pOwner) : State(pOwner) {}
    void Enter() override;
    void Update(float dt) override;
    void Draw() const override;
    void DrawShadow() const override;
    void DrawDebug() const override;
};

class EnemyFlying::StateCooldown : public Enemy::State
{
private:
    float m_StateTimer = 0.0f;
public:
    StateCooldown(EnemyFlying* pOwner) : State(pOwner) {}
    void Enter() override;
    void Update(float dt) override;
    void Draw() const override;
    void DrawShadow() const override;
    void DrawDebug() const override;
};

//======================================
// 共有リソース管理
//======================================
void EnemyFlying_InitializeShared();
void EnemyFlying_FinalizeShared();

//======================================
// 切断結果からの生成
//======================================
struct SlicedFlyingEnemyParams
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

EnemyFlying* EnemyFlying_CreateFromSlice(const SlicedFlyingEnemyParams& params);

#endif // ENEMY_FLYING_H