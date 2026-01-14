/**
 * @file map.h
 * @brief É}ÉbÉvÇÃä«óù
 * @author Natsume Shidara
 * @date 2025/11/10
 */
#ifndef MAP_H
#define MAP_H
#include <DirectXMath.h>
#include "collision.h"

void Map_Initialize();
void Map_Finalize();

void Map_Update(double elapsed_time);
void Map_Draw();

struct MapObject
{
    int KindId;
    DirectX::XMFLOAT3 Posision;
    AABB Aabb;
};

const MapObject* Map_GetObject(int index);

int Map_GetObjectCount();
void Map_DrawShadow();

#endif
