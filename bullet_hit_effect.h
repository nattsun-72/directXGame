/**
 * @file camera.h
 * @brief ÉJÉÅÉâêßå‰
 *
 * @author Natsume Shidara
 * @date 2025/09/11
 */

#ifndef BULLET_HIT_EFFECT_H
#define BULLET_HIT_EFFECT_H

#include <DirectXMath.h>

void BulletHItEffect_Initialize();
void BulletHItEffect_Finalize(void);
void BulletHItEffect_Update(double elapsed_time);
void BulletHItEffect_Create(const DirectX::XMFLOAT3 position);

void BulletHItEffect_Draw();


#endif // BULLET_HIT_EFFECT_H
