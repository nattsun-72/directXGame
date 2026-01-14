/**
 * @file Skybox.h
 * @brief スカイボックス表示
 * @author Natsume Shidara
 * @date 2025/11/21
 */

#include "skybox.h"
#include "model.h"
#include <DirectXMath.h>
#include <d3d11.h>
#include "shader3d_unlit.h"

using namespace DirectX;

static MODEL* g_pModel = nullptr;
static XMFLOAT3 g_Position {};

void Skybox_Initialize() { 
	g_pModel = ModelLoad("assets/fbx/sky.fbx", 300.0f, true); 
}

void Skybox_Finalize(void) { 
	ModelRelease(g_pModel);

}

void Skybox_SetPosition(const DirectX::XMFLOAT3& position) {
	g_Position = position;
}

void Skybox_Draw() { 
    ModelUnlitDraw(g_pModel,XMMatrixTranslationFromVector(XMLoadFloat3(&g_Position)));
}
