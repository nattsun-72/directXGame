/**
 * @file Skybox.h
 * @brief スカイボックス表示
 * @author Natsume Shidara
 * @date 2025/11/21
 */

#ifndef SKYBOX_H
#define SKYBOX_H

#include <DirectXMath.h>
#include <d3d11.h>

void Skybox_Initialize();
void Skybox_Finalize(void);
void Skybox_SetPosition(const DirectX::XMFLOAT3& position);
void Skybox_Draw();

#endif // SKYBOX_H
