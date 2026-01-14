/**
 * @file grid.h
 * @brief XZ平面グリッドの表示
 * 
 * @author Natsume Shidara
 * @date 2025/09/11
 */

#ifndef GRID_H
#define GRID_H

#include <d3d11.h>

void Grid_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Grid_Finalize(void);
void Grid_Draw(void);

#endif // GRID_H
