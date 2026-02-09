/****************************************
 * @file particle_test.h
 * @brief パーティクル用クラス宣言ヘッダ
 *
 * @author
 *   Natsume Shidara
 * @date
 *   2026/02/04
 ****************************************/
#ifndef PARTICLE_TEST_H
#define PARTICLE_TEST_H

#include "particle.h"
#include "texture.h"
#include <random>

class NormalParticle : public Particle {
private:
    int m_texId = -1;
    float m_scale = 1.0f;
    float m_alpha = 1.0f;

public:
    NormalParticle(int texId, const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& velocity, double lifeTime)
        : Particle(position, velocity, lifeTime), m_texId(texId) {
    }

    void Update(double elapsedTime) override;
    void Draw() const override;
};

class NormalEmitter : public Emitter {
private:
    

    int m_texId = -1;
    std::mt19937 m_mt{ std::random_device{}() };
    

protected:
    Particle* CreateParticle() override;

public:
    NormalEmitter(size_t capacity, const DirectX::XMVECTOR& position, double particlesPerSecond, bool isEmit = false)
        : Emitter(capacity, position, particlesPerSecond, isEmit), m_texId(Texture_Load(L"assets/effect000.jpg")) {
    }
};

#endif // PARTICLE_TEST_H