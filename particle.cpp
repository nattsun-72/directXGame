#include "particle.h"

void Emitter::Update(double elapsedTime)
{
	m_accumulatedTime += elapsedTime;

	const double secPerParticle = 1.0 / m_particlesPerSecond;

	//•¬oˆ—

	while (m_accumulatedTime >= secPerParticle){
		if (m_particleCount >= m_particleCapacity) break;

		if (m_isEmit) {
			m_particles[m_particleCount++] = CreateParticle();
		}

		m_accumulatedTime -= secPerParticle;
	}

	for(int i=0;i<m_particleCount;i++){
		m_particles[i]->Update(elapsedTime);
	}

	for (int i = m_particleCount - 1; i >= 0; i--) {
		if (m_particles[i]->IsDestroy()) {
			delete m_particles[i];
			m_particles[i] = m_particles[--m_particleCount];
		}
	}
}

void Emitter::Draw() const
{
	for (int i = 0; i < m_particleCount; i++) {
		m_particles[i]->Draw();
	}
}
