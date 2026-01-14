#include "bullet.h"
#include "model.h"
#include "trail.h"
using namespace DirectX;

class Bullet
{
private:
    XMFLOAT3 m_position{};
    XMFLOAT3 m_velocity{};
    double m_accumulatedTime{ 0.0 };
    static constexpr double MAX_LIFE_TIME = 3.0;

public:
    Bullet(const XMFLOAT3& position, const XMFLOAT3& velocity) : m_position(position), m_velocity(velocity) {}

    void Update(double elapsed_time)
    {
        m_accumulatedTime += elapsed_time;
        XMStoreFloat3(&m_position, XMLoadFloat3(&m_position) + XMLoadFloat3(&m_velocity) * static_cast<float>(elapsed_time));
        Trail_Create(m_position, {0.2f,0.2f,0.6f,0.15f},2.0f,1.0f);
    }

    const XMFLOAT3& GetPosition() const { return m_position; }

    XMFLOAT3 GetFront() const
    {
        XMFLOAT3 front;
        XMStoreFloat3(&front, XMVector3Normalize(XMLoadFloat3(&m_velocity)));
        return front;
    }

    const bool IsDestroy() const { return m_accumulatedTime > MAX_LIFE_TIME; }
};

static constexpr int MAX_BULLET = 2048;
static Bullet* g_pBullets[MAX_BULLET]{};
static int g_BulletCount{ 0 };
static MODEL* g_pBulletModel{ nullptr };

void Bullet_Initialize()
{
    //g_pBulletModel = ModelLoad("assets/fbx/IRR_CAN/ST_IRR_CAN001_bullet.fbx",10.0f);

    // 全ての弾丸を削除して初期化
    for (int i = 0; i < MAX_BULLET; i++)
    {
        if (g_pBullets[i] != nullptr)
        {
            delete g_pBullets[i];
            g_pBullets[i] = nullptr;
        }
    }
    g_BulletCount = 0;
}

void Bullet_Finalize()
{
    for (int i = 0; i < g_BulletCount; i++)
    {
        if (g_pBullets[i] != nullptr)
        {
            delete g_pBullets[i];
            g_pBullets[i] = nullptr;
        }
    }
    g_BulletCount = 0;
    ModelRelease(g_pBulletModel);
    g_pBulletModel = nullptr;
}

void Bullet_Update(double elapsed_time)
{
    // 先に全ての弾丸を更新
    for (int i = 0; i < g_BulletCount; i++)
    {
        g_pBullets[i]->Update(elapsed_time);
    }

    // 寿命切れの弾丸を削除(後ろからループして配列の再配置問題を回避)
    for (int i = g_BulletCount - 1; i >= 0; i--)
    {
        if (g_pBullets[i]->IsDestroy())
        {
            Bullet_Destory(i);
        }
    }
}

void Bullet_Draw()
{
    XMMATRIX mtxWorld;
    for (int i = 0; i < g_BulletCount; i++)
    {
        XMVECTOR position = XMLoadFloat3(&g_pBullets[i]->GetPosition());
        mtxWorld = XMMatrixTranslationFromVector(position);
        ModelDraw(g_pBulletModel, mtxWorld);
    }
}

void Bullet_Create(const XMFLOAT3& position, const XMFLOAT3& velocity)
{
    if (g_BulletCount >= MAX_BULLET)
    {
        return;
    }

    // 静的解析のために明示的にインデックスを分離
    int index = g_BulletCount;
    if (index < MAX_BULLET) // 追加の安全性チェック
    {
        g_pBullets[index] = new Bullet(position, velocity);
        g_BulletCount++;
    }
}

void Bullet_Destory(int index)
{
    delete g_pBullets[index];
    g_pBullets[index] = g_pBullets[g_BulletCount - 1];
    g_pBullets[g_BulletCount - 1] = nullptr;
    g_BulletCount--;
}

int Bullet_GetBulletCount() { return g_BulletCount; }

const AABB Bullet_GetAABB(int index) { return Model_GetAABB(g_pBulletModel, g_pBullets[index]->GetPosition()); }

const XMFLOAT3& Bullet_GetPosition(int index) { return g_pBullets[index]->GetPosition(); }
