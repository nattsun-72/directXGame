/****************************************
 * @file ray.cpp
 * @brief レイ（光線）の実装
 * @author Natsume Shidara
 * @date 2025/12/05
 * @update 2025/12/05
 ****************************************/

#include "ray.h"
#include "direct3d.h" // Direct3D_ScreenToWorldを使用するため
#include <cmath>

using namespace DirectX;

//======================================
// コンストラクタ
//======================================
Ray::Ray()
    : m_origin(0.0f, 0.0f, 0.0f), m_direction(0.0f, 0.0f, 1.0f)
{
}

Ray::Ray(const XMFLOAT3& origin, const XMFLOAT3& direction)
    : m_origin(origin), m_direction(direction)
{
}

//======================================
// ファクトリメソッド
//======================================
Ray Ray::CreateFromScreen(float screenX, float screenY, const XMFLOAT4X4& view, const XMFLOAT4X4& proj)
{
    // スクリーン座標からNear平面(0.0f)とFar平面(1.0f)のワールド座標を算出
    XMFLOAT3 nearPos = Direct3D_ScreenToWorld(static_cast <int>(screenX), static_cast <int>(screenY), 0.0f, view, proj);
    XMFLOAT3 farPos = Direct3D_ScreenToWorld(static_cast <int>(screenX), static_cast <int>(screenY), 1.0f, view, proj);

    // 方向ベクトル算出 (Far - Near)
    XMVECTOR vNear = XMLoadFloat3(&nearPos);
    XMVECTOR vFar = XMLoadFloat3(&farPos);
    XMVECTOR vDir = vFar - vNear;

    // 正規化
    vDir = XMVector3Normalize(vDir);

    XMFLOAT3 dir;
    XMStoreFloat3(&dir, vDir);

    // 始点はNear平面の位置とする
    return Ray(nearPos, dir);
}

//======================================
// 判定・計算関数
//======================================
bool Ray::IntersectsFloor(XMFLOAT3* outHitPos, float floorY) const
{
    // レイの方向ベクトルのY成分
    float dirY = m_direction.y;

    // 誤差対策：レイがほぼ水平の場合は交差なしとみなす
    if (std::abs(dirY) < 0.0001f)
    {
        return false;
    }

    // 高さの差分
    float heightDiff = floorY - m_origin.y;

    // 距離比率（t）を計算: t = (TargetY - OriginY) / DirY
    float t = heightDiff / dirY;

    // t < 0 の場合はレイの進行方向の逆（背後）にあるため交差しないとみなす
    if (t < 0.0f)
    {
        return false;
    }

    // 交差点 = Origin + Dir * t
    XMVECTOR vOrigin = XMLoadFloat3(&m_origin);
    XMVECTOR vDir = XMLoadFloat3(&m_direction);
    XMVECTOR vHit = vOrigin + vDir * t;

    if (outHitPos)
    {
        XMStoreFloat3(outHitPos, vHit);
    }

    return true;
}