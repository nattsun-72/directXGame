/****************************************
 * @file particle_test.cpp
 * @brief パーティクルテスト実装（修正版）
 *
 * @author
 *   Natsume Shidara
 * @date
 *   2026/02/04
 * @update
 *   2026/02/06 - 深度書き込み無効化、描画改善
 ****************************************/
#include "particle_test.h"
#include "billboard.h"
using namespace DirectX;

void NormalParticle::Update(double elapsedTime)
{
    // 速度に時間を掛けて移動量を計算
    XMVECTOR deltaPos = XMVectorScale(GetVelocity(), static_cast<float>(elapsedTime));
    AddPosition(deltaPos);

    // 3. ライフタイムに応じてフェードアウト
    float lifeRatio = std::fminf(GetAccumulatedTime() / GetLifeTime(), 1.0f);
    m_alpha = 1.0f - lifeRatio; // 寿命が減るほど透明に
    //m_scale = 1.0f - lifeRatio;
    m_scale = 1.0f;

    // 基底クラスの更新(ライフタイム減少)
    Particle::Update(elapsedTime);
}

void NormalParticle::Draw() const
{
    XMFLOAT3 position{};
    XMStoreFloat3(&position, GetPosition());
    XMFLOAT4 color = {1.0f,1.0f,1.0f,m_alpha};
    Billboard_Draw(m_texId, position, { m_scale, m_scale }, { 0.0f, 0.0f }, color);

    
}

Particle* NormalEmitter::CreateParticle()
{
    static constexpr float MIN_DIRECTION = -DirectX::XM_PI;
    static constexpr float MAX_DIRECTION = DirectX::XM_PI;
    static constexpr float MIN_RADIUS = 0.0f;
    static constexpr float MAX_RADIUS = 0.5f;
    static constexpr float MIN_SPEED = 2.0f;
    static constexpr float MAX_SPEED = 5.0f;
    static constexpr float MIN_LIFE_TIME = 0.5f;
    static constexpr float MAX_LIFE_TIME = 1.5f;
    static constexpr float SPAWN_HEIGHT = 0.2f;      // 生成位置の高さ
    static constexpr float UPWARD_VELOCITY = 1.0f;   // 上方向の速度
    static constexpr float VERTICAL_SPREAD = 0.5f;   // 上下方向のばらつき

    std::uniform_real_distribution<float> m_dirDist{ MIN_DIRECTION, MAX_DIRECTION };
    std::uniform_real_distribution<float> m_radiusDist{ MIN_RADIUS, MAX_RADIUS };
    std::uniform_real_distribution<float> m_speedDist{ MIN_SPEED, MAX_SPEED };
    std::uniform_real_distribution<float> m_lifeTimeDist{ MIN_LIFE_TIME, MAX_LIFE_TIME };
    std::uniform_real_distribution<float> m_verticalDist{ -VERTICAL_SPREAD, VERTICAL_SPREAD };

    // XZ平面上の角度
    float angle = m_dirDist(m_mt);
    float radius = m_radiusDist(m_mt);

    // 放射状の初期位置（エミッター位置から少しオフセット）
    XMVECTOR spawnOffset = XMVectorSet(
        cosf(angle) * radius,
        SPAWN_HEIGHT,
        sinf(angle) * radius,
        0.0f
    );

    // パーティクルの初期位置
    XMVECTOR particlePos = GetPosition() + spawnOffset;

    // 方向ベクトル（外側+上向き）
    XMVECTOR direction = XMVectorSet(
        cosf(angle),                          // X方向（外側へ）
        UPWARD_VELOCITY + m_verticalDist(m_mt), // Y方向（上向き）
        sinf(angle),                          // Z方向（外側へ）
        0.0f
    );
    direction = XMVector3Normalize(direction);

    // 速度ベクトル
    float speed = m_speedDist(m_mt);
    XMVECTOR velocity = XMVectorScale(direction, speed);

    // ライフタイム
    float lifeTime = m_lifeTimeDist(m_mt);

    return new NormalParticle(m_texId, particlePos, velocity, lifeTime);
}