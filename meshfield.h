/****************************************
 * @file meshfield.h
 * @brief メッシュフィールド（地形）生成・描画
 * @author Natsume Shidara
 * @date 2025/12/10
 * @update 2026/01/13 - 地形隆起機能追加
 ****************************************/

#ifndef MESHFIELD_H
#define MESHFIELD_H

#include <d3d11.h>
#include <DirectXMath.h>

 //======================================
 // 地形設定
 //======================================
namespace TerrainConfig
{
    // メッシュ分割数
    constexpr int MESH_H_COUNT = 100;  // X軸方向
    constexpr int MESH_V_COUNT = 100;  // Z軸方向

    // メッシュ1マスのサイズ
    constexpr float MESH_SIZE = 2.5f;

    // 地形サイズ（自動計算）
    constexpr float TERRAIN_WIDTH = MESH_H_COUNT * MESH_SIZE;
    constexpr float TERRAIN_DEPTH = MESH_V_COUNT * MESH_SIZE;

    // 高さ設定
    constexpr float MAX_HEIGHT = 1.5f;       // 最大隆起高さ
    constexpr float MIN_HEIGHT = -0.3f;      // 最低高さ（くぼみ）
    constexpr float EDGE_FLATTEN_MARGIN = 5.0f;  // 端を平坦にするマージン
}

//======================================
// 公開関数
//======================================

/**
 * @brief メッシュフィールド初期化
 */
void MeshField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

/**
 * @brief メッシュフィールド終了処理
 */
void MeshField_Finalize();

/**
 * @brief メッシュフィールド更新
 */
void MeshField_Update(double elapsed_time);

/**
 * @brief メッシュフィールド描画
 */
void MeshField_Draw(const DirectX::XMMATRIX& mtxWorld);

/**
 * @brief メッシュフィールドシャドウ描画
 */
void MeshField_DrawShadow(const DirectX::XMMATRIX& mtxWorld);

/**
 * @brief 指定座標の地形高さを取得
 * @param x X座標（ワールド）
 * @param z Z座標（ワールド）
 * @return 地形の高さ（Y座標）
 */
float MeshField_GetHeight(float x, float z);

/**
 * @brief 指定座標の地形法線を取得
 * @param x X座標（ワールド）
 * @param z Z座標（ワールド）
 * @return 法線ベクトル
 */
DirectX::XMFLOAT3 MeshField_GetNormal(float x, float z);

/**
 * @brief 地形の幅を取得
 */
float MeshField_GetWidth();

/**
 * @brief 地形の奥行きを取得
 */
float MeshField_GetDepth();

#endif // MESHFIELD_H