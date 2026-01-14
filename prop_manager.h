/****************************************
 * @file prop_manager.h
 * @brief 動的オブジェクト(プロップ)の管理クラス
 * @author NatsumeShidara
 * @update 2025/12/16
 * @update 2026/01/12 - 外部からの破片追加機能
 ****************************************/

#ifndef PROP_MANAGER_H
#define PROP_MANAGER_H

#include "ray.h"
#include "model.h"
#include "collider.h"
#include <DirectXMath.h>
#include <vector>

 //======================================
 // 破片追加用パラメータ
 //======================================
struct SlicedPieceParams
{
    std::vector<MeshData> meshes;
    MODEL* originalModel;
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 velocity;
    DirectX::XMFLOAT3 planeNormal;
    float mass;
    float rootVolume;
    float lifeTime;          // 消滅までの時間（-1で無限）
    ColliderType colliderType;
    bool isFrontSide;
};

//======================================
// プロップ管理関数群
//======================================

void PropManager_Initialize();
void PropManager_Finalize();
void PropManager_Update(double elapsed_time);
void PropManager_Draw();
void PropManager_DrawShadow();
void PropManager_DrawDebug();

/**
 * @brief オブジェクトの切断を試みる
 */
void PropManager_TrySlice(const Ray& startRay, const Ray& endRay);

/**
 * @brief 外部から切断破片を追加する
 * @param params 破片のパラメータ
 * @detail 敵の切断後などに呼び出して、残骸をPropとして登録する
 */
void PropManager_AddSlicedPiece(const SlicedPieceParams& params);

#endif // PROP_MANAGER_H