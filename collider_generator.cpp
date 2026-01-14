/****************************************
 * @file collider_generator.cpp
 * @brief コライダー自動生成の実装
 * @detail DirectXCollisionを利用したOBB算出
 * @author Natsume Shidara
 * @update 2025/12/15
 ****************************************/

#include "collider_generator.h"
#include <vector>
#include <DirectXCollision.h> // BoundingOrientedBox用

using namespace DirectX;

//======================================
// OBB生成処理
//======================================
Collider ColliderGenerator::GenerateBestFit(const MODEL* pModel, ColliderType colliderType)
{
    Collider col;
    col.type = colliderType;

    // モデルが無効ならデフォルト値を返却
    if (!pModel || pModel->Meshes.empty())
    {
        col.obb.center = XMFLOAT3(0, 0, 0);
        col.obb.extents = XMFLOAT3(1, 1, 1);
        XMStoreFloat4(&col.obb.orientation, XMQuaternionIdentity());
        return col;
    }

    // ---------------------------------------------------
    // 1. 全頂点の抽出
    // ---------------------------------------------------
    std::vector<XMFLOAT3> points;

    // 全メッシュの頂点を1つのリストに集約
    for (const auto& mesh : pModel->Meshes)
    {
        points.reserve(points.size() + mesh.vertices.size());
        for (const auto& v : mesh.vertices)
        {
            points.push_back(v.position);
        }
    }

    if (points.empty())
    {
        col.obb.center = XMFLOAT3(0, 0, 0);
        col.obb.extents = XMFLOAT3(0.5f, 0.5f, 0.5f);
        XMStoreFloat4(&col.obb.orientation, XMQuaternionIdentity());
        return col;
    }

    // ---------------------------------------------------
    // 2. 最適なOBBの計算 (DirectXCollision利用)
    // ---------------------------------------------------
    BoundingOrientedBox dxObb;

    // CreateFromPointsは内部で共分散行列を用いて
    // 点群にフィットする姿勢とサイズを自動計算してくれる
    BoundingOrientedBox::CreateFromPoints(dxObb, points.size(), points.data(), sizeof(XMFLOAT3));

    // ---------------------------------------------------
    // 3. 結果の格納
    // ---------------------------------------------------
    col.obb.center = dxObb.Center;
    col.obb.extents = dxObb.Extents;
    col.obb.orientation = dxObb.Orientation;

    return col;
}