/****************************************
 * @file ray.h
 * @brief レイ（光線）の定義と衝突判定
 * @author Natsume Shidara
 * @date 2025/12/05
 * @update 2025/12/05
 ****************************************/

#ifndef RAY_H
#define RAY_H

#include <DirectXMath.h>

 //--------------------------------------
 // クラス宣言
 //--------------------------------------
 /****************************************
  * @class Ray
  * @brief 3次元空間上の半直線（レイ）を管理するクラス
  *
  * 始点と方向ベクトルを持ち、スクリーン座標からのレイ生成や
  * 平面（地面）との交差判定を行う機能を提供する。
  ****************************************/
class Ray {
private:
    //--------------------------------------
    // メンバ変数群
    //--------------------------------------
    DirectX::XMFLOAT3 m_origin;    // 始点
    DirectX::XMFLOAT3 m_direction; // 方向（正規化済み単位ベクトル）

public:
    //======================================
    // コンストラクタ
    //======================================
    Ray();
    Ray(const DirectX::XMFLOAT3& origin, const DirectX::XMFLOAT3& direction);

    //======================================
    // ファクトリメソッド（生成補助）
    //======================================
    /**
     * @brief スクリーン座標からワールド空間へのレイを作成する
     * @param screenX マウス等のスクリーンX座標
     * @param screenY マウス等のスクリーンY座標
     * @param view ビュー行列
     * @param proj プロジェクション行列
     * @return 生成されたRayオブジェクト
     */
    static Ray CreateFromScreen(float screenX, float screenY, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj);

    //======================================
    // 判定・計算関数
    //======================================
    /**
     * @brief 特定の高さ（Y平面）との交差点を計算する
     * @param[out] outHitPos 交差位置を格納する変数のポインタ
     * @param floorY 平面の高さ（デフォルトは0.0f）
     * @return 交差した場合はtrue、平行などで交差しない場合はfalse
     * @detail
     * レイが指定したY座標の平面と交わる位置を計算する。
     * レイが水平に近い場合や、平面がレイの背後にある場合はfalseを返す。
     */
    bool IntersectsFloor(DirectX::XMFLOAT3* outHitPos, float floorY = 0.0f) const;

    // ゲッター
    DirectX::XMFLOAT3 GetOrigin() const { return m_origin; }
    DirectX::XMFLOAT3 GetDirection() const { return m_direction; }
};

#endif // RAY_H