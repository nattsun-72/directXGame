/****************************************
 * @file particle.h
 * @brief パーティクル用クラス宣言ヘッダ
 *
 * @author
 *   Natsume Shidara
 * @date
 *   2026/02/04
 ****************************************/
#ifndef PARTICLE_H
#define PARTICLE_H
#include <DirectXMath.h>

 //インスタンシング
 //GPUレンダリング(GPUパーティクル)
 //ジオメトリシェーダ

class Particle {
private:
    DirectX::XMVECTOR m_position{};
    DirectX::XMVECTOR m_velocity{};
    //float m_mass; 一旦無視
    double m_lifeTime{};
    double m_accumulatedTime{};

protected:
    void Destroy() {
        m_lifeTime = 0.0;
    }

    // Position
    void SetPosition(const DirectX::XMVECTOR& position) {
        m_position = position;
    }
    DirectX::XMVECTOR GetPosition() const {
        return m_position;
    }
    void AddPosition(const DirectX::XMVECTOR& v) {
        m_position = DirectX::XMVectorAdd(m_position, v);
    }

    void SetVelocity(const DirectX::XMVECTOR& velocity) {
        m_velocity = velocity;
    }
    DirectX::XMVECTOR GetVelocity() const {
        return m_velocity;
    }
    void AddVelocity(const DirectX::XMVECTOR& v) {
        m_velocity = DirectX::XMVectorAdd(m_velocity, v);
    }

    void SetLifeTime(double lifeTime) {
        m_lifeTime = lifeTime;
    }
    double GetLifeTime() const {
        return m_lifeTime;
    }

    double GetAccumulatedTime() const {
        return m_accumulatedTime;
    }

public:
    Particle(const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& velocity, double lifeTime)
        : m_position{ position }, m_velocity{ velocity }, m_lifeTime{ lifeTime } {
    }
    virtual ~Particle() = default;

    virtual bool IsDestroy() const {
        return m_lifeTime <= 0.0;
    }

    virtual void Update(double elapsedTime) {
        m_accumulatedTime += elapsedTime;

        if(m_accumulatedTime >= m_lifeTime){
            Destroy();
        }

    }

    virtual void Draw() const = 0;
};

class Emitter {
private:
    DirectX::XMVECTOR m_position{};
    double m_particlesPerSecond{};
    double m_accumulatedTime = 0.0;
    bool m_isEmit = false;

protected:
    size_t m_particleCapacity{};
    size_t m_particleCount{};
    Particle** m_particles{};

    virtual Particle* CreateParticle() = 0;

    // Position
    void SetPosition(const DirectX::XMVECTOR& position) {
        m_position = position;
    }
    DirectX::XMVECTOR GetPosition() const {
        return m_position;
    }

    // ParticlesPerSecond
    void SetParticlesPerSecond(double pps) {
        m_particlesPerSecond = pps;
    }
    double GetParticlesPerSecond() const {
        return m_particlesPerSecond;
    }

    double GetAccumulatedTime() const {
        return m_accumulatedTime;
    }

    // IsEmit
    bool IsEmitting() const {
        return m_isEmit;
    }

public:
    Emitter(size_t capacity, const DirectX::XMVECTOR& position, double particlesPerSecond, bool isEmit = false)
        : m_particleCapacity(capacity), m_position(position), m_particlesPerSecond(particlesPerSecond), m_isEmit(isEmit)
    {
        m_particles = new Particle * [m_particleCapacity];
        for (size_t i = 0; i < m_particleCapacity; i++) {
            m_particles[i] = nullptr;
        }
    }

    virtual ~Emitter() {
        for (size_t i = 0; i < m_particleCount; i++) {
            delete m_particles[i];
        }
        delete[] m_particles;
    }

    virtual void Update(double elapsedTime);
    virtual void Draw() const;

    void Emit(bool isEmit) { m_isEmit = isEmit; }
};

#endif // PARTICLE_H