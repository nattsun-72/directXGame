/****************************************
 * @file    collision.cpp
 * @brief   コリジョン判定実装完全版（警告対応版）
 *
 ****************************************/

#include "collision.h"
#include "ray.h" 
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <cfloat> // FLT_MAX
#include <vector>

#include "collider.h"

using namespace DirectX;

// =================================================================
// 2D / 簡易判定
// =================================================================

bool Collision_IsOverlapCircle(const Circle& a, const Circle& b)
{
    float dx = b.center.x - a.center.x;
    float dy = b.center.y - a.center.y;
    float rSum = a.radius + b.radius;
    return (dx * dx + dy * dy) <= (rSum * rSum);
}

bool Collision_IsOverlapBox(const Box& a, const Box& b)
{
    float aLeft = a.center.x - a.half_width;
    float aRight = a.center.x + a.half_width;
    float aTop = a.center.y - a.half_height;
    float aBottom = a.center.y + a.half_height;

    float bLeft = b.center.x - b.half_width;
    float bRight = b.center.x + b.half_width;
    float bTop = b.center.y - b.half_height;
    float bBottom = b.center.y + b.half_height;

    return (aLeft < bRight && aRight > bLeft && aTop < bBottom && aBottom > bTop);
}

bool Collision_IsOverlapAABB(const AABB& a, const AABB& b) {
    return (
        a.min.x <= b.max.x && a.max.x >= b.min.x &&
        a.min.y <= b.max.y && a.max.y >= b.min.y &&
        a.min.z <= b.max.z && a.max.z >= b.min.z);
}

bool Collision_IsOverlapSphere(const Sphere& a, const XMFLOAT3& point)
{
    XMVECTOR ac = XMLoadFloat3(&a.center);
    XMVECTOR bc = XMLoadFloat3(&point);
    XMVECTOR lsq = XMVector3LengthSq(bc - ac);
    float rSq = a.radius * a.radius;
    return rSq > XMVectorGetX(lsq);
}

// =================================================================
// AABB 詳細判定
// =================================================================

Hit Collision_IsHitAABB(const AABB& a, const AABB& b)
{
    Hit hit{};

    // 1. 重なりチェック
    if (!Collision_IsOverlapAABB(a, b)) return hit;

    hit.isHit = true;

    // 2. 深度計算
    float xDepth = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    float yDepth = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
    float zDepth = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

    // 3. 最小深度の軸を選択
    bool shallowX = (xDepth <= yDepth && xDepth <= zDepth);
    bool shallowY = (yDepth < xDepth && yDepth <= zDepth);

    XMFLOAT3 centerA = a.GetCenter();
    XMFLOAT3 centerB = b.GetCenter();
    XMVECTOR normal = XMVectorZero();

    if (shallowX) {
        hit.depth = xDepth;
        normal = (centerA.x < centerB.x) ? XMVectorSet(-1, 0, 0, 0) : XMVectorSet(1, 0, 0, 0);
    }
    else if (shallowY) {
        hit.depth = yDepth;
        normal = (centerA.y < centerB.y) ? XMVectorSet(0, -1, 0, 0) : XMVectorSet(0, 1, 0, 0);
    }
    else { // shallowZ
        hit.depth = zDepth;
        normal = (centerA.z < centerB.z) ? XMVectorSet(0, 0, -1, 0) : XMVectorSet(0, 0, 1, 0);
    }
    XMStoreFloat3(&hit.normal, normal);

    // 4. 接点計算 (Intersection Volume Center)
    XMVECTOR vMinA = XMLoadFloat3(&a.min);
    XMVECTOR vMaxA = XMLoadFloat3(&a.max);
    XMVECTOR vMinB = XMLoadFloat3(&b.min);
    XMVECTOR vMaxB = XMLoadFloat3(&b.max);

    XMVECTOR vInterMin = XMVectorMax(vMinA, vMinB);
    XMVECTOR vInterMax = XMVectorMin(vMaxA, vMaxB);
    XMVECTOR vContact = (vInterMin + vInterMax) * 0.5f;

    XMStoreFloat3(&hit.contactPoint, vContact);

    return hit;
}

// =================================================================
// 三角形・球 関連ヘルパー
// =================================================================

// 点 p と三角形 tri の最近接点を求める
XMFLOAT3 Collision_ClosestPointTriangle(const XMFLOAT3& point, const Triangle& tri)
{
    XMVECTOR p = XMLoadFloat3(&point);
    XMVECTOR a = XMLoadFloat3(&tri.p0);
    XMVECTOR b = XMLoadFloat3(&tri.p1);
    XMVECTOR c = XMLoadFloat3(&tri.p2);

    XMVECTOR ab = b - a;
    XMVECTOR ac = c - a;
    XMVECTOR ap = p - a;

    float d1 = XMVectorGetX(XMVector3Dot(ab, ap));
    float d2 = XMVectorGetX(XMVector3Dot(ac, ap));

    if (d1 <= 0.0f && d2 <= 0.0f) {
        XMFLOAT3 ret; XMStoreFloat3(&ret, a); return ret; // A点
    }

    XMVECTOR bp = p - b;
    float d3 = XMVectorGetX(XMVector3Dot(ab, bp));
    float d4 = XMVectorGetX(XMVector3Dot(ac, bp));

    if (d3 >= 0.0f && d4 <= d3) {
        XMFLOAT3 ret; XMStoreFloat3(&ret, b); return ret; // B点
    }

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        XMFLOAT3 ret; XMStoreFloat3(&ret, a + v * ab); return ret; // AB上
    }

    XMVECTOR cp = p - c;
    float d5 = XMVectorGetX(XMVector3Dot(ab, cp));
    float d6 = XMVectorGetX(XMVector3Dot(ac, cp));

    if (d6 >= 0.0f && d5 <= d6) {
        XMFLOAT3 ret; XMStoreFloat3(&ret, c); return ret; // C点
    }

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        XMFLOAT3 ret; XMStoreFloat3(&ret, a + w * ac); return ret; // AC上
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        XMFLOAT3 ret; XMStoreFloat3(&ret, b + w * (c - b)); return ret; // BC上
    }

    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    XMFLOAT3 ret; XMStoreFloat3(&ret, a + ab * v + ac * w); return ret; // 面内部
}

// 球 vs 三角形
Hit Collision_IsHitSphereTriangle(const Sphere& sphere, const Triangle& tri)
{
    Hit hit = {};

    XMFLOAT3 closest = Collision_ClosestPointTriangle(sphere.center, tri);

    XMVECTOR vCenter = XMLoadFloat3(&sphere.center);
    XMVECTOR vClosest = XMLoadFloat3(&closest);
    XMVECTOR vDiff = vCenter - vClosest;

    float distSq = XMVectorGetX(XMVector3LengthSq(vDiff));
    float radiusSq = sphere.radius * sphere.radius;

    if (distSq < radiusSq)
    {
        hit.isHit = true;
        hit.contactPoint = closest;

        float dist = std::sqrt(distSq);
        hit.depth = sphere.radius - dist;

        if (dist > 1e-5f) {
            XMStoreFloat3(&hit.normal, vDiff / dist);
        }
        else {
            hit.normal = tri.normal;
        }
    }
    return hit;
}

// =================================================================
// SAT (OBB) ヘルパー関数
// =================================================================

// 軸 axis 上での OBB の投影半径を計算
static float GetOBBProjectedRadius(const XMVECTOR& axis, const XMVECTOR& u0, const XMVECTOR& u1, const XMVECTOR& u2, const XMFLOAT3& extents)
{
    return extents.x * std::abs(XMVectorGetX(XMVector3Dot(axis, u0))) +
        extents.y * std::abs(XMVectorGetX(XMVector3Dot(axis, u1))) +
        extents.z * std::abs(XMVectorGetX(XMVector3Dot(axis, u2)));
}

static void GetTriangleProjection(const XMVECTOR& axis, const XMVECTOR& v0, const XMVECTOR& v1, const XMVECTOR& v2, float& outMin, float& outMax)
{
    float d0 = XMVectorGetX(XMVector3Dot(axis, v0));
    float d1 = XMVectorGetX(XMVector3Dot(axis, v1));
    float d2 = XMVectorGetX(XMVector3Dot(axis, v2));
    outMin = std::min({ d0, d1, d2 });
    outMax = std::max({ d0, d1, d2 });
}

static XMVECTOR ClosestPointOnSegmentToSegment(XMVECTOR p1, XMVECTOR q1, XMVECTOR p2, XMVECTOR q2)
{
    XMVECTOR d1 = q1 - p1;
    XMVECTOR d2 = q2 - p2;
    XMVECTOR r = p1 - p2;
    float a = XMVectorGetX(XMVector3Dot(d1, d1));
    float e = XMVectorGetX(XMVector3Dot(d2, d2));
    float f = XMVectorGetX(XMVector3Dot(d2, r));

    if (a <= 1e-6f && e <= 1e-6f) return p1;
    if (a <= 1e-6f) return p1;

    float c = XMVectorGetX(XMVector3Dot(d1, r));
    if (e <= 1e-6f) {
        float t = std::clamp(-c / a, 0.0f, 1.0f);
        return p1 + d1 * t;
    }

    float b = XMVectorGetX(XMVector3Dot(d1, d2));
    float denom = a * e - b * b;
    float s = 0.0f;
    if (denom != 0.0f) {
        s = std::clamp((b * f - c * e) / denom, 0.0f, 1.0f);
    }
    float t = (b * s + f) / e;

    if (t < 0.0f) { t = 0.0f; s = std::clamp(-c / a, 0.0f, 1.0f); }
    else if (t > 1.0f) { t = 1.0f; s = std::clamp((b - c) / a, 0.0f, 1.0f); }

    XMVECTOR c1 = p1 + d1 * s;
    XMVECTOR c2 = p2 + d2 * t;
    return (c1 + c2) * 0.5f;
}

// =================================================================
// OBB vs Triangle (SAT Main)
// =================================================================

Hit Collision_IsHitOBBTriangle(const OBB& obb, const Triangle& tri)
{
    Hit hit;
    hit.isHit = false;
    hit.depth = FLT_MAX;

    // 1. データ準備
    XMVECTOR obbCenter = XMLoadFloat3(&obb.center);
    XMMATRIX obbRot = XMMatrixRotationQuaternion(XMLoadFloat4(&obb.orientation));
    XMVECTOR u0 = obbRot.r[0];
    XMVECTOR u1 = obbRot.r[1];
    XMVECTOR u2 = obbRot.r[2];

    XMVECTOR v0 = XMLoadFloat3(&tri.p0);
    XMVECTOR v1 = XMLoadFloat3(&tri.p1);
    XMVECTOR v2 = XMLoadFloat3(&tri.p2);
    XMVECTOR triNorm = XMLoadFloat3(&tri.normal);

    // 軸: OBB軸(3) + 面法線(1) + エッジ外積(9) = 13本
    XMVECTOR e0 = v1 - v0;
    XMVECTOR e1 = v2 - v1;
    XMVECTOR e2 = v0 - v2;

    XMVECTOR axes[13];
    axes[0] = u0; axes[1] = u1; axes[2] = u2;
    axes[3] = triNorm;
    axes[4] = XMVector3Cross(u0, e0); axes[5] = XMVector3Cross(u0, e1); axes[6] = XMVector3Cross(u0, e2);
    axes[7] = XMVector3Cross(u1, e0); axes[8] = XMVector3Cross(u1, e1); axes[9] = XMVector3Cross(u1, e2);
    axes[10] = XMVector3Cross(u2, e0); axes[11] = XMVector3Cross(u2, e1); axes[12] = XMVector3Cross(u2, e2);

    int bestAxis = -1;

    // 2. 分離軸テスト
    for (int i = 0; i < 13; i++)
    {
        XMVECTOR axis = axes[i];
        if (XMVectorGetX(XMVector3LengthSq(axis)) < 1e-6f) continue;
        axis = XMVector3Normalize(axis);

        float obbProj = XMVectorGetX(XMVector3Dot(axis, obbCenter));
        float r = GetOBBProjectedRadius(axis, u0, u1, u2, obb.extents);
        float obbMin = obbProj - r;
        float obbMax = obbProj + r;

        float triMin, triMax;
        GetTriangleProjection(axis, v0, v1, v2, triMin, triMax);

        // 重なりなし -> 分離
        if (obbMax < triMin || triMax < obbMin) return hit;

        // 重なり量
        float o1 = obbMax - triMin;
        float o2 = triMax - obbMin;
        float overlap;

        if (o1 < o2) {
            overlap = o1;
        }
        else {
            overlap = o2;
            axis = -axis;
        }

        if (overlap < hit.depth) {
            hit.depth = overlap;
            XMStoreFloat3(&hit.normal, axis);
            bestAxis = i;
        }
    }

    hit.isHit = true;
    XMVECTOR vNormal = XMLoadFloat3(&hit.normal);

    // 3. 接触点推定
    XMVECTOR contactVec = XMVectorZero();

    if (bestAxis == 3) { // Face: Triangle
        XMVECTOR dir = -vNormal;
        float sx = (XMVectorGetX(XMVector3Dot(dir, u0)) >= 0) ? obb.extents.x : -obb.extents.x;
        float sy = (XMVectorGetX(XMVector3Dot(dir, u1)) >= 0) ? obb.extents.y : -obb.extents.y;
        float sz = (XMVectorGetX(XMVector3Dot(dir, u2)) >= 0) ? obb.extents.z : -obb.extents.z;
        contactVec = obbCenter + u0 * sx + u1 * sy + u2 * sz;
    }
    else if (bestAxis <= 2) { // Face: OBB
        float d0 = XMVectorGetX(XMVector3Dot(-vNormal, v0));
        float d1 = XMVectorGetX(XMVector3Dot(-vNormal, v1));
        float d2 = XMVectorGetX(XMVector3Dot(-vNormal, v2));
        if (d0 >= d1 && d0 >= d2) contactVec = v0;
        else if (d1 >= d2)        contactVec = v1;
        else                      contactVec = v2;
    }
    else { // Edge-Edge
        int edgeIdx = bestAxis - 4;
        int obbAx = edgeIdx / 3;
        int triAx = edgeIdx % 3;

        XMVECTOR p1, q1;
        if (triAx == 0) { p1 = v0; q1 = v1; }
        else if (triAx == 1) { p1 = v1; q1 = v2; }
        else { p1 = v2; q1 = v0; }

        XMVECTOR obbDir = axes[obbAx];
        XMVECTOR toTri = vNormal;
        XMVECTOR centerToFace = XMVectorZero();

        for (int k = 0; k < 3; ++k) {
            if (k == obbAx) continue;
            float s = (XMVectorGetX(XMVector3Dot(toTri, axes[k])) > 0) ? 1.0f : -1.0f;
            float e = (k == 0 ? obb.extents.x : (k == 1 ? obb.extents.y : obb.extents.z));
            centerToFace += axes[k] * (e * s);
        }
        float eSize = (obbAx == 0 ? obb.extents.x : (obbAx == 1 ? obb.extents.y : obb.extents.z));
        XMVECTOR p2 = obbCenter + centerToFace - obbDir * eSize;
        XMVECTOR q2 = obbCenter + centerToFace + obbDir * eSize;

        contactVec = ClosestPointOnSegmentToSegment(p1, q1, p2, q2);
    }

    XMStoreFloat3(&hit.contactPoint, contactVec);
    return hit;
}

// =================================================================
// レイ判定 (Ray)
// =================================================================

bool Collision_IntersectRaySphere(const Ray& ray, const Sphere& sphere, float* outDist)
{
    XMFLOAT3 origin = ray.GetOrigin();
    XMFLOAT3 dir = ray.GetDirection();
    XMVECTOR m = XMLoadFloat3(&origin) - XMLoadFloat3(&sphere.center);
    XMVECTOR d = XMLoadFloat3(&dir);

    float b = XMVectorGetX(XMVector3Dot(m, d));
    float c = XMVectorGetX(XMVector3Dot(m, m)) - (sphere.radius * sphere.radius);

    if (c > 0.0f && b > 0.0f) return false;
    float disc = b * b - c;
    if (disc < 0.0f) return false;

    float t = -b - std::sqrt(disc);
    if (t < 0.0f) t = 0.0f;

    if (outDist) *outDist = t;
    return true;
}

bool Collision_IntersectRayAABB(const Ray& ray, const AABB& aabb, float* outDist)
{
    XMFLOAT3 origin = ray.GetOrigin();
    XMFLOAT3 dir = ray.GetDirection();
    float tMin = 0.0f;
    float tMax = FLT_MAX;

    // X
    if (std::abs(dir.x) < 1e-6f) {
        if (origin.x < aabb.min.x || origin.x > aabb.max.x) return false;
    }
    else {
        float inv = 1.0f / dir.x;
        float t1 = (aabb.min.x - origin.x) * inv;
        float t2 = (aabb.max.x - origin.x) * inv;
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) return false;
    }
    // Y
    if (std::abs(dir.y) < 1e-6f) {
        if (origin.y < aabb.min.y || origin.y > aabb.max.y) return false;
    }
    else {
        float inv = 1.0f / dir.y;
        float t1 = (aabb.min.y - origin.y) * inv;
        float t2 = (aabb.max.y - origin.y) * inv;
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) return false;
    }
    // Z
    if (std::abs(dir.z) < 1e-6f) {
        if (origin.z < aabb.min.z || origin.z > aabb.max.z) return false;
    }
    else {
        float inv = 1.0f / dir.z;
        float t1 = (aabb.min.z - origin.z) * inv;
        float t2 = (aabb.max.z - origin.z) * inv;
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) return false;
    }

    if (outDist) *outDist = tMin;
    return true;
}

// =================================================================
// Sphere vs OBB
// =================================================================
Hit Collision_IsHitSphereOBB(const Sphere& sphere, const OBB& obb)
{
    Hit hit;

    XMVECTOR vSphereCenter = XMLoadFloat3(&sphere.center);
    XMVECTOR vOBBCenter = XMLoadFloat3(&obb.center);
    XMVECTOR vDelta = vSphereCenter - vOBBCenter;

    XMMATRIX mRot = XMMatrixRotationQuaternion(XMLoadFloat4(&obb.orientation));
    XMMATRIX mInvRot = XMMatrixTranspose(mRot);

    XMVECTOR vLocalDelta = XMVector3TransformNormal(vDelta, mInvRot);
    XMVECTOR vLocalExtents = XMLoadFloat3(&obb.extents);
    XMVECTOR vLocalClosest = XMVectorClamp(vLocalDelta, -vLocalExtents, vLocalExtents);

    XMVECTOR vWorldClosest = XMVector3TransformNormal(vLocalClosest, mRot) + vOBBCenter;

    XMVECTOR vDiff = vSphereCenter - vWorldClosest;
    float distSq = XMVectorGetX(XMVector3LengthSq(vDiff));
    float radiusSq = sphere.radius * sphere.radius;

    if (distSq < radiusSq)
    {
        hit.isHit = true;
        float dist = std::sqrt(distSq);
        hit.depth = sphere.radius - dist;
        XMStoreFloat3(&hit.contactPoint, vWorldClosest);

        if (dist > 1e-6f)
        {
            XMStoreFloat3(&hit.normal, vDiff / dist);
        }
        else
        {
            XMStoreFloat3(&hit.normal, mRot.r[1]);
        }
    }
    return hit;
}

// =================================================================
// OBB vs OBB (SAT: 15軸判定)
// =================================================================
Hit Collision_IsHitOBBOBB(const OBB& a, const OBB& b)
{
    Hit hit;
    hit.isHit = false;
    hit.depth = FLT_MAX;

    XMVECTOR cA = XMLoadFloat3(&a.center);
    XMVECTOR cB = XMLoadFloat3(&b.center);
    XMMATRIX rA = XMMatrixRotationQuaternion(XMLoadFloat4(&a.orientation));
    XMMATRIX rB = XMMatrixRotationQuaternion(XMLoadFloat4(&b.orientation));

    XMVECTOR uA[3] = { rA.r[0], rA.r[1], rA.r[2] };
    XMVECTOR uB[3] = { rB.r[0], rB.r[1], rB.r[2] };

    XMVECTOR axes[15];
    int axisCount = 0;

    // 面法線 (6本)
    for (int i = 0; i < 3; i++) axes[axisCount++] = uA[i];
    for (int i = 0; i < 3; i++) axes[axisCount++] = uB[i];

    // エッジ外積 (9本)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            XMVECTOR cross = XMVector3Cross(uA[i], uB[j]);
            if (XMVectorGetX(XMVector3LengthSq(cross)) > 1e-6f) {
                axes[axisCount++] = XMVector3Normalize(cross);
            }
        }
    }

    XMVECTOR vTranslation = cB - cA;
    XMVECTOR bestAxis = XMVectorZero();

    // SAT Loop
    for (int i = 0; i < axisCount; i++)
    {
        XMVECTOR axis = axes[i];

        float dist = std::abs(XMVectorGetX(XMVector3Dot(vTranslation, axis)));

        float projA = GetOBBProjectedRadius(axis, uA[0], uA[1], uA[2], a.extents);
        float projB = GetOBBProjectedRadius(axis, uB[0], uB[1], uB[2], b.extents);

        float penetration = (projA + projB) - dist;

        if (penetration < 0.0f) return hit;

        if (penetration < hit.depth)
        {
            hit.depth = penetration;
            bestAxis = axis;
        }
    }

    hit.isHit = true;

    if (XMVectorGetX(XMVector3Dot(vTranslation, bestAxis)) < 0.0f)
    {
        bestAxis = -bestAxis;
    }
    XMStoreFloat3(&hit.normal, -bestAxis);

    XMVECTOR contact = cA + (vTranslation * 0.5f) + (bestAxis * hit.depth * 0.5f);
    XMStoreFloat3(&hit.contactPoint, contact);

    return hit;
}

// =================================================================
// OBB vs AABB
// =================================================================
Hit Collision_IsHitOBBAABB(const OBB& obb, const AABB& aabb)
{
    // AABB を回転ゼロの OBB に変換する
    OBB aabbAsObb;

    // 中心座標
    aabbAsObb.center = aabb.GetCenter();

    // extents (ハーフサイズ)
    XMVECTOR vMin = XMLoadFloat3(&aabb.min);
    XMVECTOR vMax = XMLoadFloat3(&aabb.max);
    XMVECTOR vSize = vMax - vMin;
    XMStoreFloat3(&aabbAsObb.extents, vSize * 0.5f);

    // 回転 (Identity Quaternion: 回転なし)
    aabbAsObb.orientation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

    // 既存の OBB vs OBB (SAT) に投げる
    return Collision_IsHitOBBOBB(obb, aabbAsObb);
}

// =================================================================
// カプセル用ユーティリティ関数
// =================================================================

// 線分上の最近接点を求める
XMFLOAT3 Collision_ClosestPointOnSegment(const XMFLOAT3& point,
    const XMFLOAT3& segStart,
    const XMFLOAT3& segEnd)
{
    XMVECTOR p = XMLoadFloat3(&point);
    XMVECTOR a = XMLoadFloat3(&segStart);
    XMVECTOR b = XMLoadFloat3(&segEnd);
    XMVECTOR ab = b - a;

    float t = XMVectorGetX(XMVector3Dot(p - a, ab)) / XMVectorGetX(XMVector3Dot(ab, ab));
    t = std::clamp(t, 0.0f, 1.0f);

    XMFLOAT3 result;
    XMStoreFloat3(&result, a + ab * t);
    return result;
}

// 線分と線分の最短距離を計算（オプションでパラメータs,tを返す）
float Collision_DistanceSegmentSegment(const XMFLOAT3& p1, const XMFLOAT3& q1,
    const XMFLOAT3& p2, const XMFLOAT3& q2,
    float* outS, float* outT)
{
    XMVECTOR P1 = XMLoadFloat3(&p1);
    XMVECTOR Q1 = XMLoadFloat3(&q1);
    XMVECTOR P2 = XMLoadFloat3(&p2);
    XMVECTOR Q2 = XMLoadFloat3(&q2);

    XMVECTOR d1 = Q1 - P1;
    XMVECTOR d2 = Q2 - P2;
    XMVECTOR r = P1 - P2;

    float a = XMVectorGetX(XMVector3Dot(d1, d1));
    float e = XMVectorGetX(XMVector3Dot(d2, d2));
    float f = XMVectorGetX(XMVector3Dot(d2, r));

    float s = 0.0f, t = 0.0f;

    // 両方が点の場合
    if (a <= 1e-6f && e <= 1e-6f) {
        s = t = 0.0f;
        XMFLOAT3 result;
        XMStoreFloat3(&result, P1 - P2);
        if (outS) *outS = s;
        if (outT) *outT = t;
        return std::sqrt(result.x * result.x + result.y * result.y + result.z * result.z);
    }

    // d1が点の場合
    if (a <= 1e-6f) {
        s = 0.0f;
        t = std::clamp(f / e, 0.0f, 1.0f);
    }
    // d2が点の場合
    else if (e <= 1e-6f) {
        t = 0.0f;
        float c = XMVectorGetX(XMVector3Dot(d1, r));
        s = std::clamp(-c / a, 0.0f, 1.0f);
    }
    // 一般的なケース
    else {
        float c = XMVectorGetX(XMVector3Dot(d1, r));
        float b = XMVectorGetX(XMVector3Dot(d1, d2));
        float denom = a * e - b * b;

        if (denom != 0.0f) {
            s = std::clamp((b * f - c * e) / denom, 0.0f, 1.0f);
        }
        else {
            s = 0.0f;
        }

        t = (b * s + f) / e;

        // tが範囲外の場合は再計算
        if (t < 0.0f) {
            t = 0.0f;
            s = std::clamp(-c / a, 0.0f, 1.0f);
        }
        else if (t > 1.0f) {
            t = 1.0f;
            s = std::clamp((b - c) / a, 0.0f, 1.0f);
        }
    }

    XMVECTOR c1 = P1 + d1 * s;
    XMVECTOR c2 = P2 + d2 * t;
    XMVECTOR diff = c1 - c2;

    if (outS) *outS = s;
    if (outT) *outT = t;

    return XMVectorGetX(XMVector3Length(diff));
}

// =================================================================
// カプセル vs 球
// =================================================================
Hit Collision_IsHitCapsuleSphere(const Capsule& capsule, const Sphere& sphere)
{
    Hit hit;

    // カプセルの軸線分上の最近接点を求める
    XMFLOAT3 closestOnSegment = Collision_ClosestPointOnSegment(
        sphere.center, capsule.start, capsule.end);

    // 距離を計算
    XMVECTOR vClosest = XMLoadFloat3(&closestOnSegment);
    XMVECTOR vSphereCenter = XMLoadFloat3(&sphere.center);
    XMVECTOR vDiff = vSphereCenter - vClosest;

    float distSq = XMVectorGetX(XMVector3LengthSq(vDiff));
    float radiusSum = capsule.radius + sphere.radius;
    float radiusSumSq = radiusSum * radiusSum;

    if (distSq < radiusSumSq) {
        hit.isHit = true;

        float dist = std::sqrt(distSq);
        hit.depth = radiusSum - dist;

        // 接触点：線分上の最近接点からカプセル半径分進んだ位置
        if (dist > 1e-6f) {
            XMVECTOR vNormal = vDiff / dist;
            XMStoreFloat3(&hit.normal, vNormal);
            XMStoreFloat3(&hit.contactPoint, vClosest + vNormal * capsule.radius);
        }
        else {
            // 完全に重なっている場合
            XMStoreFloat3(&hit.normal, XMVectorSet(0, 1, 0, 0));
            hit.contactPoint = closestOnSegment;
        }
    }

    return hit;
}

// =================================================================
// カプセル vs カプセル
// =================================================================
Hit Collision_IsHitCapsuleCapsule(const Capsule& a, const Capsule& b)
{
    Hit hit;

    // 2つの線分間の最短距離とパラメータを求める
    float s, t;
    float dist = Collision_DistanceSegmentSegment(
        a.start, a.end, b.start, b.end, &s, &t);

    float radiusSum = a.radius + b.radius;

    if (dist < radiusSum) {
        hit.isHit = true;
        hit.depth = radiusSum - dist;

        // 最近接点を計算
        XMVECTOR vAStart = XMLoadFloat3(&a.start);
        XMVECTOR vAEnd = XMLoadFloat3(&a.end);
        XMVECTOR vBStart = XMLoadFloat3(&b.start);
        XMVECTOR vBEnd = XMLoadFloat3(&b.end);

        XMVECTOR pointOnA = vAStart + (vAEnd - vAStart) * s;
        XMVECTOR pointOnB = vBStart + (vBEnd - vBStart) * t;

        XMVECTOR vDiff = pointOnB - pointOnA;

        if (dist > 1e-6f) {
            XMVECTOR vNormal = XMVector3Normalize(vDiff);
            XMStoreFloat3(&hit.normal, vNormal);

            // 接触点：A側の表面
            XMStoreFloat3(&hit.contactPoint, pointOnA + vNormal * a.radius);
        }
        else {
            // 完全に重なっている場合
            XMStoreFloat3(&hit.normal, XMVectorSet(0, 1, 0, 0));
            XMStoreFloat3(&hit.contactPoint, pointOnA);
        }
    }

    return hit;
}

// =================================================================
// カプセル vs OBB
// =================================================================
Hit Collision_IsHitCapsuleOBB(const Capsule& capsule, const OBB& obb)
{
    // カプセルを多数の球でサンプリングして近似する方法
    // より正確な実装にはGJKやSATが必要だが、ここでは簡易実装

    const int sampleCount = 5;
    Hit bestHit;
    bestHit.depth = -1.0f;

    XMVECTOR vStart = XMLoadFloat3(&capsule.start);
    XMVECTOR vEnd = XMLoadFloat3(&capsule.end);

    for (int i = 0; i < sampleCount; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sampleCount - 1);
        XMVECTOR vSamplePos = XMVectorLerp(vStart, vEnd, t);

        Sphere sampleSphere;
        XMStoreFloat3(&sampleSphere.center, vSamplePos);
        sampleSphere.radius = capsule.radius;

        Hit hit = Collision_IsHitSphereOBB(sampleSphere, obb);

        if (hit.isHit && hit.depth > bestHit.depth) {
            bestHit = hit;
        }
    }

    return bestHit;
}

// =================================================================
// カプセル vs AABB
// =================================================================
Hit Collision_IsHitCapsuleAABB(const Capsule& capsule, const AABB& aabb)
{
    // AABBをOBBに変換して判定
    OBB obbFromAABB;
    obbFromAABB.center = aabb.GetCenter();

    XMVECTOR vMin = XMLoadFloat3(&aabb.min);
    XMVECTOR vMax = XMLoadFloat3(&aabb.max);
    XMVECTOR vSize = vMax - vMin;
    XMStoreFloat3(&obbFromAABB.extents, vSize * 0.5f);

    obbFromAABB.orientation = XMFLOAT4(0, 0, 0, 1); // 回転なし

    return Collision_IsHitCapsuleOBB(capsule, obbFromAABB);
}

// =================================================================
// カプセル vs 三角形
// =================================================================
Hit Collision_IsHitCapsuleTriangle(const Capsule& capsule, const Triangle& tri)
{
    Hit hit;

    // 線分の始点と終点それぞれについて三角形との距離を計算
    XMFLOAT3 closestStart = Collision_ClosestPointTriangle(capsule.start, tri);
    XMFLOAT3 closestEnd = Collision_ClosestPointTriangle(capsule.end, tri);

    XMVECTOR vStart = XMLoadFloat3(&capsule.start);
    XMVECTOR vEnd = XMLoadFloat3(&capsule.end);
    XMVECTOR vClosestStart = XMLoadFloat3(&closestStart);
    XMVECTOR vClosestEnd = XMLoadFloat3(&closestEnd);

    float distStart = XMVectorGetX(XMVector3Length(vStart - vClosestStart));
    float distEnd = XMVectorGetX(XMVector3Length(vEnd - vClosestEnd));

    // より近い方を使用
    float minDist = std::min(distStart, distEnd);
    XMVECTOR closestOnCapsule = (distStart < distEnd) ? vStart : vEnd;
    XMVECTOR closestOnTri = (distStart < distEnd) ? vClosestStart : vClosestEnd;

    if (minDist < capsule.radius) {
        hit.isHit = true;
        hit.depth = capsule.radius - minDist;

        XMVECTOR vDiff = closestOnCapsule - closestOnTri;

        if (minDist > 1e-6f) {
            XMStoreFloat3(&hit.normal, XMVector3Normalize(vDiff));
        }
        else {
            hit.normal = tri.normal;
        }

        XMStoreFloat3(&hit.contactPoint, closestOnTri);
    }

    return hit;
}

// =================================================================
// レイ vs カプセル
// =================================================================
bool Collision_IntersectRayCapsule(const Ray& ray, const Capsule& capsule, float* outDist)
{
    XMFLOAT3 origin = ray.GetOrigin();
    XMFLOAT3 dir = ray.GetDirection();

    XMVECTOR vOrigin = XMLoadFloat3(&origin);
    XMVECTOR vDir = XMLoadFloat3(&dir);
    XMVECTOR vStart = XMLoadFloat3(&capsule.start);
    XMVECTOR vEnd = XMLoadFloat3(&capsule.end);

    XMVECTOR vAxis = vEnd - vStart;
    float axisLen = XMVectorGetX(XMVector3Length(vAxis));

    if (axisLen < 1e-6f) {
        // カプセルが球に退化している場合
        Sphere sphere;
        sphere.center = capsule.start;
        sphere.radius = capsule.radius;
        return Collision_IntersectRaySphere(ray, sphere, outDist);
    }

    vAxis = vAxis / axisLen;

    // レイとカプセル軸の最近接距離を計算
    XMVECTOR vDelta = vOrigin - vStart;
    float rayDot = XMVectorGetX(XMVector3Dot(vDir, vAxis));
    float deltaDot = XMVectorGetX(XMVector3Dot(vDelta, vAxis));

    // 簡易実装：両端の球との交差判定
    Sphere sphereStart, sphereEnd;
    sphereStart.center = capsule.start;
    sphereStart.radius = capsule.radius;
    sphereEnd.center = capsule.end;
    sphereEnd.radius = capsule.radius;

    float distStart, distEnd;
    bool hitStart = Collision_IntersectRaySphere(ray, sphereStart, &distStart);
    bool hitEnd = Collision_IntersectRaySphere(ray, sphereEnd, &distEnd);

    if (hitStart || hitEnd) {
        if (outDist) {
            if (hitStart && hitEnd) {
                *outDist = std::min(distStart, distEnd);
            }
            else if (hitStart) {
                *outDist = distStart;
            }
            else {
                *outDist = distEnd;
            }
        }
        return true;
    }

    return false;
}

/****************************************
 * @brief   統合判定関数 Collision_Detect の拡張版
 *          カプセル、AABB、三角形に対応
 *
 * collision.cpp の末尾に追加または置き換え
 ****************************************/

Hit Collision_Detect(const Collider& colA, const Collider& colB)
{
    using CT = ColliderType;

    // =================================================================
    // Sphere 系
    // =================================================================
    if (colA.type == CT::Sphere)
    {
        if (colB.type == CT::Sphere)
        {
            // Sphere vs Sphere
            if (Collision_IsOverlapSphere(colA.sphere, colB.sphere.center)) {
                Hit hit;
                hit.isHit = true;

                XMVECTOR vA = XMLoadFloat3(&colA.sphere.center);
                XMVECTOR vB = XMLoadFloat3(&colB.sphere.center);
                XMVECTOR diff = vA - vB;

                float dist = XMVectorGetX(XMVector3Length(diff));
                hit.depth = (colA.sphere.radius + colB.sphere.radius) - dist;

                if (dist > 1e-6f) {
                    XMStoreFloat3(&hit.normal, XMVector3Normalize(diff));
                    XMStoreFloat3(&hit.contactPoint, vB + (diff / dist) * colB.sphere.radius);
                }
                else {
                    XMStoreFloat3(&hit.normal, XMVectorSet(0, 1, 0, 0));
                    hit.contactPoint = colB.sphere.center;
                }
                return hit;
            }
        }
        else if (colB.type == CT::Box)
        {
            // Sphere vs OBB
            return Collision_IsHitSphereOBB(colA.sphere, colB.obb);
        }
        else if (colB.type == CT::AABB)
        {
            // Sphere vs AABB（OBB化して判定）
            OBB aabbAsOBB;
            aabbAsOBB.center = colB.aabb.GetCenter();
            XMFLOAT3 colB_AABB_Size = colB.aabb.GetSize();
            XMVECTOR vSize = XMLoadFloat3(&colB_AABB_Size);
            XMStoreFloat3(&aabbAsOBB.extents, vSize * 0.5f);
            aabbAsOBB.orientation = XMFLOAT4(0, 0, 0, 1);
            return Collision_IsHitSphereOBB(colA.sphere, aabbAsOBB);
        }
        else if (colB.type == CT::Capsule)
        {
            // Sphere vs Capsule（逆順で呼んで法線を反転）
            Hit hit = Collision_IsHitCapsuleSphere(colB.capsule, colA.sphere);
            if (hit.isHit) {
                XMVECTOR n = XMLoadFloat3(&hit.normal);
                XMStoreFloat3(&hit.normal, -n);
            }
            return hit;
        }
        else if (colB.type == CT::Triangle)
        {
            // Sphere vs Triangle
            return Collision_IsHitSphereTriangle(colA.sphere, colB.triangle);
        }
    }

    // =================================================================
    // Box (OBB) 系
    // =================================================================
    else if (colA.type == CT::Box)
    {
        if (colB.type == CT::Sphere)
        {
            // OBB vs Sphere（逆順で呼んで法線を反転）
            Hit hit = Collision_IsHitSphereOBB(colB.sphere, colA.obb);
            if (hit.isHit) {
                XMVECTOR n = XMLoadFloat3(&hit.normal);
                XMStoreFloat3(&hit.normal, -n);
            }
            return hit;
        }
        else if (colB.type == CT::Box)
        {
            // OBB vs OBB
            return Collision_IsHitOBBOBB(colA.obb, colB.obb);
        }
        else if (colB.type == CT::AABB)
        {
            // OBB vs AABB
            return Collision_IsHitOBBAABB(colA.obb, colB.aabb);
        }
        else if (colB.type == CT::Capsule)
        {
            // OBB vs Capsule（逆順で呼んで法線を反転）
            Hit hit = Collision_IsHitCapsuleOBB(colB.capsule, colA.obb);
            if (hit.isHit) {
                XMVECTOR n = XMLoadFloat3(&hit.normal);
                XMStoreFloat3(&hit.normal, -n);
            }
            return hit;
        }
        else if (colB.type == CT::Triangle)
        {
            // OBB vs Triangle
            return Collision_IsHitOBBTriangle(colA.obb, colB.triangle);
        }
    }

    // =================================================================
    // AABB 系
    // =================================================================
    else if (colA.type == CT::AABB)
    {
        if (colB.type == CT::Sphere)
        {
            // AABB vs Sphere（OBB化）
            OBB aabbAsOBB;
            aabbAsOBB.center = colA.aabb.GetCenter();
            XMFLOAT3 colA_AABB_Size = colA.aabb.GetSize();
            XMVECTOR vSize = XMLoadFloat3(&colA_AABB_Size);
            XMStoreFloat3(&aabbAsOBB.extents, vSize * 0.5f);
            aabbAsOBB.orientation = XMFLOAT4(0, 0, 0, 1);

            Hit hit = Collision_IsHitSphereOBB(colB.sphere, aabbAsOBB);
            if (hit.isHit) {
                XMVECTOR n = XMLoadFloat3(&hit.normal);
                XMStoreFloat3(&hit.normal, -n);
            }
            return hit;
        }
        else if (colB.type == CT::Box)
        {
            // AABB vs OBB（逆順）
            Hit hit = Collision_IsHitOBBAABB(colB.obb, colA.aabb);
            if (hit.isHit) {
                XMVECTOR n = XMLoadFloat3(&hit.normal);
                XMStoreFloat3(&hit.normal, -n);
            }
            return hit;
        }
        else if (colB.type == CT::AABB)
        {
            // AABB vs AABB
            return Collision_IsHitAABB(colA.aabb, colB.aabb);
        }
        else if (colB.type == CT::Capsule)
        {
            // AABB vs Capsule（逆順で呼んで法線を反転）
            Hit hit = Collision_IsHitCapsuleAABB(colB.capsule, colA.aabb);
            if (hit.isHit) {
                XMVECTOR n = XMLoadFloat3(&hit.normal);
                XMStoreFloat3(&hit.normal, -n);
            }
            return hit;
        }
        else if (colB.type == CT::Triangle)
        {
            // AABB vs Triangle（OBB化してから判定）
            OBB aabbAsOBB;
            aabbAsOBB.center = colA.aabb.GetCenter();
            XMFLOAT3 colA_AABB_Size = colA.aabb.GetSize();
            XMVECTOR vSize = XMLoadFloat3(&colA_AABB_Size);
            XMStoreFloat3(&aabbAsOBB.extents, vSize * 0.5f);
            aabbAsOBB.orientation = XMFLOAT4(0, 0, 0, 1);
            return Collision_IsHitOBBTriangle(aabbAsOBB, colB.triangle);
        }
    }

    // =================================================================
    // Capsule 系
    // =================================================================
    else if (colA.type == CT::Capsule)
    {
        if (colB.type == CT::Sphere)
        {
            // Capsule vs Sphere
            return Collision_IsHitCapsuleSphere(colA.capsule, colB.sphere);
        }
        else if (colB.type == CT::Box)
        {
            // Capsule vs OBB
            return Collision_IsHitCapsuleOBB(colA.capsule, colB.obb);
        }
        else if (colB.type == CT::AABB)
        {
            // Capsule vs AABB
            return Collision_IsHitCapsuleAABB(colA.capsule, colB.aabb);
        }
        else if (colB.type == CT::Capsule)
        {
            // Capsule vs Capsule
            return Collision_IsHitCapsuleCapsule(colA.capsule, colB.capsule);
        }
        else if (colB.type == CT::Triangle)
        {
            // Capsule vs Triangle
            return Collision_IsHitCapsuleTriangle(colA.capsule, colB.triangle);
        }
    }

    // =================================================================
    // Triangle 系
    // =================================================================
    else if (colA.type == CT::Triangle)
    {
        if (colB.type == CT::Sphere)
        {
            // Triangle vs Sphere（逆順）
            Hit hit = Collision_IsHitSphereTriangle(colB.sphere, colA.triangle);
            if (hit.isHit) {
                XMVECTOR n = XMLoadFloat3(&hit.normal);
                XMStoreFloat3(&hit.normal, -n);
            }
            return hit;
        }
        else if (colB.type == CT::Box)
        {
            // Triangle vs OBB（逆順）
            Hit hit = Collision_IsHitOBBTriangle(colB.obb, colA.triangle);
            if (hit.isHit) {
                XMVECTOR n = XMLoadFloat3(&hit.normal);
                XMStoreFloat3(&hit.normal, -n);
            }
            return hit;
        }
        else if (colB.type == CT::AABB)
        {
            // Triangle vs AABB（OBB化して逆順）
            OBB aabbAsOBB;
            aabbAsOBB.center = colB.aabb.GetCenter();
            XMFLOAT3 cloB_AABB_Size = colB.aabb.GetSize();
            XMVECTOR vSize = XMLoadFloat3(&cloB_AABB_Size);
            XMStoreFloat3(&aabbAsOBB.extents, vSize * 0.5f);
            aabbAsOBB.orientation = XMFLOAT4(0, 0, 0, 1);

            Hit hit = Collision_IsHitOBBTriangle(aabbAsOBB, colA.triangle);
            if (hit.isHit) {
                XMVECTOR n = XMLoadFloat3(&hit.normal);
                XMStoreFloat3(&hit.normal, -n);
            }
            return hit;
        }
        else if (colB.type == CT::Capsule)
        {
            // Triangle vs Capsule（逆順）
            Hit hit = Collision_IsHitCapsuleTriangle(colB.capsule, colA.triangle);
            if (hit.isHit) {
                XMVECTOR n = XMLoadFloat3(&hit.normal);
                XMStoreFloat3(&hit.normal, -n);
            }
            return hit;
        }
        else if (colB.type == CT::Triangle)
        {
            // Triangle vs Triangle は未実装
            // 必要に応じて SAT や GJK で実装
            return Hit{};
        }
    }

    // 未対応の組み合わせ
    return Hit{};
}