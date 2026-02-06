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

public:
    NormalParticle(int texId, const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& velocity, double lifeTime)
        : Particle(position, velocity, lifeTime), m_texId(texId) {
    }

    void Update(double elapsedTime) override;
    void Draw() const override;
};

class NormalEmitter : public Emitter {
private:
    static constexpr float MIN_DIRECTION = -DirectX::XM_PI;
    static constexpr float MAX_DIRECTION = DirectX::XM_PI;

    static constexpr float MIN_SPEED = 1.0f;
    static constexpr float MAX_SPEED = 5.0f;

    static constexpr float MIN_LIFE_TIME = 0.5f;
    static constexpr float MAX_LIFE_TIME = 1.0f;

    int m_texId = -1;
    std::mt19937 m_mt{ std::random_device{}() };
    std::uniform_real_distribution<float> m_dirDist{ MIN_DIRECTION, MAX_DIRECTION };
    std::uniform_real_distribution<float> m_speedDist{ MIN_SPEED, MAX_SPEED };
    std::uniform_real_distribution<float> m_lifeTimeDist{ MIN_LIFE_TIME, MAX_LIFE_TIME };

protected:
    Particle* CreateParticle() override;

public:
    NormalEmitter(size_t capacity, const DirectX::XMVECTOR& position, double particlesPerSecond, bool isEmit = false)
        : Emitter(capacity, position, particlesPerSecond, isEmit), m_texId(Texture_Load(L"assets/effect000.jpg")) {
    }
};

#endif // PARTICLE_TEST_H