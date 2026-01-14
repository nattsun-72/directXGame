/****************************************
 * @file debug_renderer.h
 * @brief デバッグ用プリミティブ描画モジュール
 * @author Natsume Shidara
 * @date 2025/12/19
 ****************************************/

#ifndef DEBUG_RENDERER_H
#define DEBUG_RENDERER_H

 //--------------------------------------
 // インクルードガード／依存ヘッダ
 //--------------------------------------
#include <DirectXMath.h>
#include <vector>
#include "collision.h"

//--------------------------------------
// クラス・名前空間宣言
//--------------------------------------
namespace DebugRenderer
{
    //======================================
    // 基本動作関数群
    //======================================

    /**
     * @brief 初期化処理
     * @detail DirectX11リソース（バッファ、シェーダー）の生成
     */
    void Initialize();

    /**
     * @brief 終了処理
     * @detail 確保したリソースの解放
     */
    void Finalize();

    /**
     * @brief 描画実行
     * @param view ビュー行列
     * @param proj プロジェクション行列
     */
    void Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj);

    //======================================
    // 形状登録関数群
    //======================================

    /**
     * @brief 線分の描画登録
     */
    void DrawLine(const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT4& color);

    /**
     * @brief AABBの描画登録
     */
    void DrawAABB(const AABB& aabb, const DirectX::XMFLOAT4& color);

    /**
     * @brief OBBの描画登録
     */
    void DrawOBB(const OBB& obb, const DirectX::XMFLOAT4& color);

    /**
     * @brief 球の描画登録
     */
    void DrawSphere(const Sphere& sphere, const DirectX::XMFLOAT4& color);

    /**
     * @brief グリッドの描画登録
     */
    void DrawGrid(float size, int divisions, const DirectX::XMFLOAT4& color);
}

#endif // DEBUG_RENDERER_H