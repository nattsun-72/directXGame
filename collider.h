/****************************************
 * @file    collider.h
 * @brief   統合コライダーシステム（拡張版）
 * @author  Natsume Shidara
 * @date    2025/01/02
 ****************************************/
#ifndef COLLIDER_H
#define COLLIDER_H

#include <DirectXMath.h>
#include "collision.h" // Sphere, OBB, AABBなどの定義
#include <algorithm>

 //--------------------------------------
 // コライダーの種類
 //--------------------------------------
enum class ColliderType
{
    Sphere,   // 球形
    Box,      // OBB（有向境界ボックス）
    AABB,     // 軸並行境界ボックス
    Capsule,  // カプセル（円柱の両端が半球）
    Triangle, // 三角形面
};

//--------------------------------------
// カプセル形状の定義
//--------------------------------------
/** @struct Capsule @brief 3D カプセル形状 */
struct Capsule {
    DirectX::XMFLOAT3 start;  // 線分の始点
    DirectX::XMFLOAT3 end;    // 線分の終点
    float radius;             // 半径

    // コンストラクタ
    Capsule() : start{ 0,0,0 }, end{ 0,1,0 }, radius(0.5f) {}
    Capsule(const DirectX::XMFLOAT3& s, const DirectX::XMFLOAT3& e, float r)
        : start(s), end(e), radius(r) {
    }

    // 高さと半径から垂直カプセルを作成
    static Capsule CreateVertical(const DirectX::XMFLOAT3& center, float height, float radius) {
        float halfHeight = height * 0.5f;
        return Capsule(
            DirectX::XMFLOAT3(center.x, center.y - halfHeight, center.z),
            DirectX::XMFLOAT3(center.x, center.y + halfHeight, center.z),
            radius
        );
    }

    // 中心座標を取得
    DirectX::XMFLOAT3 GetCenter() const {
        return DirectX::XMFLOAT3(
            (start.x + end.x) * 0.5f,
            (start.y + end.y) * 0.5f,
            (start.z + end.z) * 0.5f
        );
    }

    // 軸方向ベクトルを取得
    DirectX::XMFLOAT3 GetAxis() const {
        using namespace DirectX;
        XMVECTOR s = XMLoadFloat3(&start);
        XMVECTOR e = XMLoadFloat3(&end);
        XMFLOAT3 result;
        XMStoreFloat3(&result, e - s);
        return result;
    }

    // 高さを取得
    float GetHeight() const {
        using namespace DirectX;
        XMVECTOR s = XMLoadFloat3(&start);
        XMVECTOR e = XMLoadFloat3(&end);
        return XMVectorGetX(XMVector3Length(e - s));
    }
};

//--------------------------------------
// 統合コライダー構造体
//--------------------------------------
struct Collider
{
    ColliderType type;

    // 共用体でメモリを節約
    union
    {
        Sphere sphere;
        OBB obb;
        AABB aabb;
        Capsule capsule;
        Triangle triangle;
    };

    //--------------------------------------
    // コンストラクタ群
    //--------------------------------------

    // デフォルト: 半径1の球
    Collider() : type(ColliderType::Sphere) {
        sphere.center = { 0, 0, 0 };
        sphere.radius = 1.0f;
    }

    // 球形コライダー作成
    static Collider CreateSphere(const DirectX::XMFLOAT3& center, float radius) {
        Collider col;
        col.type = ColliderType::Sphere;
        col.sphere.center = center;
        col.sphere.radius = radius;
        return col;
    }

    // OBBコライダー作成
    static Collider CreateOBB(const DirectX::XMFLOAT3& center,
        const DirectX::XMFLOAT3& extents,
        const DirectX::XMFLOAT4& orientation = { 0,0,0,1 }) {
        Collider col;
        col.type = ColliderType::Box;
        col.obb.center = center;
        col.obb.extents = extents;
        col.obb.orientation = orientation;
        return col;
    }

    // AABBコライダー作成（最小・最大点から）
    static Collider CreateAABB(const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max) {
        Collider col;
        col.type = ColliderType::AABB;
        col.aabb.min = min;
        col.aabb.max = max;
        return col;
    }

    // AABBコライダー作成（中心とサイズから）
    static Collider CreateAABBFromCenterSize(const DirectX::XMFLOAT3& center,
        const DirectX::XMFLOAT3& size) {
        DirectX::XMFLOAT3 halfSize = {
            size.x * 0.5f,
            size.y * 0.5f,
            size.z * 0.5f
        };
        return CreateAABB(
            DirectX::XMFLOAT3(center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z),
            DirectX::XMFLOAT3(center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z)
        );
    }

    // カプセルコライダー作成
    static Collider CreateCapsule(const DirectX::XMFLOAT3& start,
        const DirectX::XMFLOAT3& end,
        float radius) {
        Collider col;
        col.type = ColliderType::Capsule;
        col.capsule.start = start;
        col.capsule.end = end;
        col.capsule.radius = radius;
        return col;
    }

    // 垂直カプセルコライダー作成（キャラクター用）
    static Collider CreateVerticalCapsule(const DirectX::XMFLOAT3& center,
        float height,
        float radius) {
        Collider col;
        col.type = ColliderType::Capsule;
        col.capsule = Capsule::CreateVertical(center, height, radius);
        return col;
    }

    // 三角形コライダー作成
    static Collider CreateTriangle(const DirectX::XMFLOAT3& p0,
        const DirectX::XMFLOAT3& p1,
        const DirectX::XMFLOAT3& p2,
        const DirectX::XMFLOAT3& normal) {
        Collider col;
        col.type = ColliderType::Triangle;
        col.triangle.p0 = p0;
        col.triangle.p1 = p1;
        col.triangle.p2 = p2;
        col.triangle.normal = normal;
        return col;
    }

    //--------------------------------------
    // ユーティリティ関数
    //--------------------------------------

    // 中心座標を取得
    DirectX::XMFLOAT3 GetCenter() const {
        switch (type) {
        case ColliderType::Sphere:
            return sphere.center;
        case ColliderType::Box:
            return obb.center;
        case ColliderType::AABB:
            return aabb.GetCenter();
        case ColliderType::Capsule:
            return capsule.GetCenter();
        case ColliderType::Triangle: {
            using namespace DirectX;
            XMVECTOR v0 = XMLoadFloat3(&triangle.p0);
            XMVECTOR v1 = XMLoadFloat3(&triangle.p1);
            XMVECTOR v2 = XMLoadFloat3(&triangle.p2);
            XMFLOAT3 result;
            XMStoreFloat3(&result, (v0 + v1 + v2) / 3.0f);
            return result;
        }
        default:
            return DirectX::XMFLOAT3(0, 0, 0);
        }
    }

    // 位置を設定（中心座標を移動）
    void SetPosition(const DirectX::XMFLOAT3& pos) {
        using namespace DirectX;

        switch (type) {
        case ColliderType::Sphere:
            sphere.center = pos;
            break;
        case ColliderType::Box:
            obb.center = pos;
            break;
        case ColliderType::AABB: {
            XMFLOAT3 size = aabb.GetSize();
            aabb.min = DirectX::XMFLOAT3(
                pos.x - size.x * 0.5f,
                pos.y - size.y * 0.5f,
                pos.z - size.z * 0.5f
            );
            aabb.max = DirectX::XMFLOAT3(
                pos.x + size.x * 0.5f,
                pos.y + size.y * 0.5f,
                pos.z + size.z * 0.5f
            );
            break;
        }
        case ColliderType::Capsule: {
            XMFLOAT3 oldCenter = capsule.GetCenter();
            XMVECTOR offset = XMLoadFloat3(&pos) - XMLoadFloat3(&oldCenter);
            XMVECTOR s = XMLoadFloat3(&capsule.start);
            XMVECTOR e = XMLoadFloat3(&capsule.end);
            XMStoreFloat3(&capsule.start, s + offset);
            XMStoreFloat3(&capsule.end, e + offset);
            break;
        }
        case ColliderType::Triangle: {
            XMFLOAT3 oldCenter = GetCenter();
            XMVECTOR offset = XMLoadFloat3(&pos) - XMLoadFloat3(&oldCenter);
            XMVECTOR v0 = XMLoadFloat3(&triangle.p0);
            XMVECTOR v1 = XMLoadFloat3(&triangle.p1);
            XMVECTOR v2 = XMLoadFloat3(&triangle.p2);
            XMStoreFloat3(&triangle.p0, v0 + offset);
            XMStoreFloat3(&triangle.p1, v1 + offset);
            XMStoreFloat3(&triangle.p2, v2 + offset);
            break;
        }
        }
    }

    // バウンディングボックスを取得（AABB形式）
    AABB GetBoundingBox() const {
        using namespace DirectX;

        switch (type) {
        case ColliderType::Sphere: {
            XMFLOAT3 c = sphere.center;
            float r = sphere.radius;
            return AABB{
                DirectX::XMFLOAT3(c.x - r, c.y - r, c.z - r),
                DirectX::XMFLOAT3(c.x + r, c.y + r, c.z + r)
            };
        }
        case ColliderType::Box: {
            // OBBの8頂点を計算してAABBを作成
            XMMATRIX rot = XMMatrixRotationQuaternion(XMLoadFloat4(&obb.orientation));
            XMVECTOR center = XMLoadFloat3(&obb.center);
            XMVECTOR ext = XMLoadFloat3(&obb.extents);

            XMFLOAT3 minPoint = { FLT_MAX, FLT_MAX, FLT_MAX };
            XMFLOAT3 maxPoint = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

            for (int i = 0; i < 8; ++i) {
                XMVECTOR corner = XMVectorSet(
                    (i & 1) ? obb.extents.x : -obb.extents.x,
                    (i & 2) ? obb.extents.y : -obb.extents.y,
                    (i & 4) ? obb.extents.z : -obb.extents.z,
                    0
                );
                XMVECTOR worldCorner = XMVector3Transform(corner, rot) + center;
                XMFLOAT3 wc;
                XMStoreFloat3(&wc, worldCorner);

                minPoint.x = std::min(minPoint.x, wc.x);
                minPoint.y = std::min(minPoint.y, wc.y);
                minPoint.z = std::min(minPoint.z, wc.z);
                maxPoint.x = std::max(maxPoint.x, wc.x);
                maxPoint.y = std::max(maxPoint.y, wc.y);
                maxPoint.z = std::max(maxPoint.z, wc.z);
            }
            return AABB{ minPoint, maxPoint };
        }
        case ColliderType::AABB:
            return aabb;
        case ColliderType::Capsule: {
            float r = capsule.radius;
            XMVECTOR s = XMLoadFloat3(&capsule.start);
            XMVECTOR e = XMLoadFloat3(&capsule.end);

            XMFLOAT3 minPoint, maxPoint;
            XMStoreFloat3(&minPoint, XMVectorMin(s, e) - XMVectorReplicate(r));
            XMStoreFloat3(&maxPoint, XMVectorMax(s, e) + XMVectorReplicate(r));

            return AABB{ minPoint, maxPoint };
        }
        case ColliderType::Triangle: {
            XMVECTOR v0 = XMLoadFloat3(&triangle.p0);
            XMVECTOR v1 = XMLoadFloat3(&triangle.p1);
            XMVECTOR v2 = XMLoadFloat3(&triangle.p2);

            XMFLOAT3 minPoint, maxPoint;
            XMStoreFloat3(&minPoint, XMVectorMin(XMVectorMin(v0, v1), v2));
            XMStoreFloat3(&maxPoint, XMVectorMax(XMVectorMax(v0, v1), v2));

            return AABB{ minPoint, maxPoint };
        }
        default:
            return AABB{ {0,0,0}, {0,0,0} };
        }
    }

    // デバッグ用：型名を取得
    const char* GetTypeName() const {
        switch (type) {
        case ColliderType::Sphere: return "Sphere";
        case ColliderType::Box: return "Box(OBB)";
        case ColliderType::AABB: return "AABB";
        case ColliderType::Capsule: return "Capsule";
        case ColliderType::Triangle: return "Triangle";
        default: return "Unknown";
        }
    }
};

#endif // COLLIDER_H