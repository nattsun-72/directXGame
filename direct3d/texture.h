/**
 * @file texture.h
 * @brief テクスチャ管理
 * @author Natsume Shidara
 * @date 2025/06/12
 */

#ifndef TEXTURE_H
#define TEXTURE_H

#include <d3d11.h>

void Texture_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

void Texture_Finalize(void);

/**
* @brief テクスチャ画像の読み込み
* @param pFilename 読み込みたいファイルの
* @pre Texture_Initialize()で初期化後
* @return 管理番号
* @retval -1 読み込めなかった場合
*/
int Texture_Load(const wchar_t* pFilename);

void Texture_AllRelease();

/**
* @param texid 管理番号
*/
void Texture_SetTexture(int texid, int slot = 0);

int Texture_Width(int texid);
int Texture_Height(int texid);

#endif
