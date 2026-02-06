/****************************************
 * @file particle_test.cpp
 * @brief パーティクルテスト実装
 *
 * @author
 *   Natsume Shidara
 * @date
 *   2026/02/04
 ****************************************/
#include "particle_test.h"
#include "billboard.h"
#include "direct3d.h"

using namespace DirectX;

void NormalParticle::Update(double elapsedTime)
{
    // 速度に時間を掛けて移動量を計算
    XMVECTOR deltaPos = XMVectorScale(GetVelocity(), static_cast<float>(elapsedTime));
    AddPosition(deltaPos);

    // 基底クラスの更新（ライフタイム減少）
    Particle::Update(elapsedTime);
}

void NormalParticle::Draw() const
{
    XMFLOAT3 position{};
    XMStoreFloat3(&position, GetPosition());

    // 加算合成を有効にする
    Direct3D_DepthStencilStateDepthIsEnable(true);
    Direct3D_SetBlendState(BlendMode::Add);

    Billboard_Draw(m_texId, position, { 1.0f, 1.0f });

    // 元に戻す（必要に応じて）
    Direct3D_SetBlendState(BlendMode::Alpha);
    Direct3D_DepthStencilStateDepthIsEnable(false);
}

Particle* NormalEmitter::CreateParticle()
{
    float angle = m_dirDist(m_mt);

    XMVECTOR direction = XMVectorSet(cosf(angle), sinf(angle), sinf(angle), 0.0f);
    XMVECTOR velocity = XMVectorScale(direction, m_speedDist(m_mt));

    float lifeTime = m_lifeTimeDist(m_mt);

    return new NormalParticle(m_texId, GetPosition(), velocity, lifeTime);
}