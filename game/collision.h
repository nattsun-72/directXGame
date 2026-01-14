/****************************************
 * @file    collision.h
 * @brief   コリジョン判定モジュール（カプセル対応版）
 * @author  Natsume Shidara
 * @date    2025/01/02
 ****************************************/
#ifndef COLLISION_H
#define COLLISION_H

#include <DirectXMath.h>

 // 前方宣言
class Ray;
struct Collider;
struct Capsule;

//--------------------------------------
// 形状定義構造体群
//--------------------------------------

/** @struct Circle @brief 2D 円形 */
struct Circle {
    DirectX::XMFLOAT2 center;
    float radius;
};

/** @struct Sphere @brief 3D 球形 */
struct Sphere {
    DirectX::XMFLOAT3 center;
    float radius;
};

/** @struct Box @brief 2D AABB (中心とハーフサイズ) */
struct Box {
    DirectX::XMFLOAT2 center;
    float half_width;
    float half_height;
};

/** @struct AABB @brief 3D AABB (最小値・最大値) */
struct AABB {
    DirectX::XMFLOAT3 min;
    DirectX::XMFLOAT3 max;

    DirectX::XMFLOAT3 GetCenter() const {
        return { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f };
    }
    DirectX::XMFLOAT3 GetSize() const {
        return { max.x - min.x, max.y - min.y, max.z - min.z };
    }
};

/** @struct Triangle @brief 3D 三角形面 */
struct Triangle {
    DirectX::XMFLOAT3 p0;
    DirectX::XMFLOAT3 p1;
    DirectX::XMFLOAT3 p2;
    DirectX::XMFLOAT3 normal;
};

/** @struct OBB @brief 3D 有向境界ボックス */
struct OBB {
    DirectX::XMFLOAT3 center;
    DirectX::XMFLOAT3 extents;
    DirectX::XMFLOAT4 orientation;
};

//--------------------------------------
// 衝突結果データ構造体
//--------------------------------------

struct Hit {
    bool isHit = false;
    DirectX::XMFLOAT3 normal{ 0, 0, 0 };
    float depth = 0.0f;
    DirectX::XMFLOAT3 contactPoint{ 0, 0, 0 };
};

//--------------------------------------
// 判定関数プロトタイプ
//--------------------------------------

// --- 簡易判定関数群 ---
bool Collision_IsOverlapCircle(const Circle& a, const Circle& b);
bool Collision_IsOverlapBox(const Box& a, const Box& b);
bool Collision_IsOverlapAABB(const AABB& a, const AABB& b);
bool Collision_IsOverlapSphere(const Sphere& a, const DirectX::XMFLOAT3& point);

// --- 詳細判定関数群 ---
Hit Collision_IsHitAABB(const AABB& a, const AABB& b);
Hit Collision_IsHitSphereTriangle(const Sphere& sphere, const Triangle& tri);
Hit Collision_IsHitOBBTriangle(const OBB& obb, const Triangle& tri);
Hit Collision_IsHitSphereOBB(const Sphere& sphere, const OBB& obb);
Hit Collision_IsHitOBBOBB(const OBB& a, const OBB& b);
Hit Collision_IsHitOBBAABB(const OBB& obb, const AABB& aabb);

// --- カプセル判定関数群（新規追加）---
Hit Collision_IsHitCapsuleSphere(const Capsule& capsule, const Sphere& sphere);
Hit Collision_IsHitCapsuleCapsule(const Capsule& a, const Capsule& b);
Hit Collision_IsHitCapsuleOBB(const Capsule& capsule, const OBB& obb);
Hit Collision_IsHitCapsuleAABB(const Capsule& capsule, const AABB& aabb);
Hit Collision_IsHitCapsuleTriangle(const Capsule& capsule, const Triangle& tri);

// --- レイ判定関数群 ---
bool Collision_IntersectRaySphere(const Ray& ray, const Sphere& sphere, float* outDist = nullptr);
bool Collision_IntersectRayAABB(const Ray& ray, const AABB& aabb, float* outDist = nullptr);
bool Collision_IntersectRayCapsule(const Ray& ray, const Capsule& capsule, float* outDist = nullptr);

// --- ユーティリティ ---
DirectX::XMFLOAT3 Collision_ClosestPointTriangle(const DirectX::XMFLOAT3& point, const Triangle& tri);
DirectX::XMFLOAT3 Collision_ClosestPointOnSegment(const DirectX::XMFLOAT3& point,
    const DirectX::XMFLOAT3& segStart,
    const DirectX::XMFLOAT3& segEnd);
float Collision_DistanceSegmentSegment(const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& q1,
    const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT3& q2,
    float* s = nullptr, float* t = nullptr);

// --- 統合判定 ---
Hit Collision_Detect(const Collider& colA, const Collider& colB);

#endif // COLLISION_H