/*==============================================================================

   軌跡エフェクトの描画 [trail.h]
														 Author : Natsume Shidara
														 Date   : 2025/09/03
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef TRAIL_H
#define TRAIL_H

#include <DirectXMath.h>

void Trail_Initialize();
void Trail_Finalize();

void Trail_Update(double elapsed_time);
void Trail_Draw();

void Trail_Create(const DirectX::XMFLOAT3& position,
				  const DirectX::XMFLOAT4& color,
				  float size,
				  double lifeTime);


#endif //TRAIL_H