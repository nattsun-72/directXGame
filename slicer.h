/****************************************
 * @file slicer.h
 * @brief メッシュ切断モジュール
 * @author Natsume Shidara
 * @date 2025/12/06
 ****************************************/

#ifndef SLICER_H
#define SLICER_H

#include "model.h"
#include <DirectXMath.h>
#include <vector>

 //======================================
 // メッシュ切断クラス
 //======================================
class Slicer
{
public:
    /**
     * @brief メッシュを平面で切断（従来版・同期処理）
     * @param targetModel 切断対象モデル
     * @param worldMatrix ワールド変換行列
     * @param planePoint 平面上の点
     * @param planeNormal 平面法線
     * @param outFront 表側の新規モデル（呼び出し側で解放）
     * @param outBack 裏側の新規モデル（呼び出し側で解放）
     * @return 切断成功時true
     */
    static bool Slice(
        MODEL* targetModel,
        const DirectX::XMFLOAT4X4& worldMatrix,
        const DirectX::XMFLOAT3& planePoint,
        const DirectX::XMFLOAT3& planeNormal,
        MODEL** outFront,
        MODEL** outBack
    );

    /**
     * @brief CPUデータのみ生成（非同期処理用）
     * @detail GPUバッファを作成せず、メッシュデータのみ返す
     * @param targetModel 切断対象モデル
     * @param worldMatrix ワールド変換行列
     * @param planePoint 平面上の点
     * @param planeNormal 平面法線
     * @param outFrontMeshes 表側のメッシュデータ配列
     * @param outBackMeshes 裏側のメッシュデータ配列
     * @return 切断成功時true
     */
    static bool SliceCPUOnly(
        MODEL* targetModel,
        const DirectX::XMFLOAT4X4& worldMatrix,
        const DirectX::XMFLOAT3& planePoint,
        const DirectX::XMFLOAT3& planeNormal,
        std::vector<MeshData>& outFrontMeshes,
        std::vector<MeshData>& outBackMeshes
    );

private:
    // 頂点補間
    static Vertex Interpolate(const Vertex& v1, const Vertex& v2, float t);

    // 平面との距離計算
    static float GetDistanceToPlane(const DirectX::XMFLOAT3& vertex, const DirectX::XMVECTOR& planeVector);
};

#endif // !SLICER_H
