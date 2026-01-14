/****************************************
 * @file enemy.h
 * @brief 敵キャラクタ基底クラスおよび管理システム
 * @author Natsume Shidara
 * @date 2025/11/26
 * @update 2026/01/10 - 地上型敵対応
 ****************************************/

#ifndef ENEMY_H
#define ENEMY_H

#include <DirectXMath.h>
#include "ray.h"

 //--------------------------------------
 // 敵タイプ列挙型
 //--------------------------------------
enum ENEMY_TYPE
{
    ENEMY_TYPE_GROUND,  // 地上型（遠距離攻撃）
    ENEMY_TYPE_FLYING,  // 飛行型（後で実装）
    ENEMY_TYPE_MAX
};

//--------------------------------------
// 敵基底クラス
//--------------------------------------
/****************************************
 * @class Enemy
 * @brief 敵キャラクタの基底クラス
 * @detail Stateパターンを用いた状態管理を行い、派生クラスで固有の挙動を定義する。
 ****************************************/
class Enemy
{
public:
    //======================================
    // 内部Stateクラス定義
    //======================================
    /**
     * @class State
     * @brief 敵の状態を管理する抽象基底クラス
     */
    class State
    {
    protected:
        Enemy* m_pOwner = nullptr;

    public:
        State(Enemy* pOwner) : m_pOwner(pOwner) {}
        virtual ~State() {}

        virtual void Enter() {}  // 状態開始時
        virtual void Update(float dt) = 0;
        virtual void Exit() {}   // 状態終了時
        virtual void Draw() const = 0;
        virtual void DrawShadow() const {}
        virtual void DrawDebug() const {}
    };

protected:
    State* m_pState = nullptr;
    State* m_pNextState = nullptr;

    // 共通プロパティ
    DirectX::XMFLOAT3 m_Position = { 0, 0, 0 };
    DirectX::XMFLOAT3 m_Front = { 0, 0, 1 };
    float m_VolumeRatio = 1.0f;  // 残り体積比率（切断用）
    bool m_IsDestroyed = false;

public:
    //======================================
    // コンストラクタ/デストラクタ
    //======================================
    Enemy() = default;
    virtual ~Enemy();

    //======================================
    // 基本動作関数
    //======================================
    void Update(float dt);
    void Draw() const;
    void DrawShadow() const;
    void DrawDebug() const;

    /** @brief フレーム頭で行う状態遷移の確定処理 */
    void UpdateState();

    /** @brief 次の状態を予約する */
    void ChangeState(State* pNext);

    //======================================
    // 純粋仮想関数
    //======================================
    virtual bool IsDestroy() const { return m_IsDestroyed; }

    //======================================
    // 共通アクセサ
    //======================================
    const DirectX::XMFLOAT3& GetPosition() const { return m_Position; }
    void SetPosition(const DirectX::XMFLOAT3& pos) { m_Position = pos; }

    const DirectX::XMFLOAT3& GetFront() const { return m_Front; }
    void SetFront(const DirectX::XMFLOAT3& front) { m_Front = front; }

    float GetVolumeRatio() const { return m_VolumeRatio; }

    /** @brief 切断による体積損失（0.0～1.0） */
    virtual void TakeDamage(float volumeLost);
};

//--------------------------------------
// グローバル管理関数
//--------------------------------------
void Enemy_Initialize();
void Enemy_Finalize();
void Enemy_Update(float dt);
void Enemy_Draw();
void Enemy_DrawShadow();
void Enemy_DrawDebug();

void Enemy_Create(ENEMY_TYPE type, const DirectX::XMFLOAT3& position);
void Enemy_Clear();

int Enemy_GetCount();
int Enemy_GetAliveCount();
Enemy* Enemy_GetEnemy(int index);

//--------------------------------------
// 切断処理
//--------------------------------------
class Ray;  // 前方宣言

/** @brief 指定レイと平面法線で敵の切断を試みる */
void Enemy_TrySlice(const Ray& ray, const DirectX::XMFLOAT3& planeNormal);

/** @brief 非同期切断の完了結果を処理する（毎フレーム呼ぶ） */
bool Enemy_ProcessSliceResults();

#endif // ENEMY_H