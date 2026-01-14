/****************************************
 * @file slicer.cpp
 * @brief メッシュ切断実装（Ear Clipping対応・AABBカリング最適化版）
 * @author Natsume Shidara
 * @date 2025/12/08
 * @update 2025/12/17 (Optimization applied)
 ****************************************/

#include "slicer.h"
#include <algorithm>
#include <cmath>
#include <list>
#include <map>
#include <vector>
#include <float.h> // FLT_MAX用

using namespace DirectX;

//--------------------------------------
// 内部用構造体・定数
//--------------------------------------

const float EPSILON = 1e-4f;

struct RawEdge
{
    XMFLOAT3 start;
    XMFLOAT3 end;
};

// 2D計算用
struct Vector2
{
    float x, y;
};

// AABB判定結果
enum class PlaneSide
{
    Front,
    Back,
    Intersect
};

//--------------------------------------
// ヘルパー関数（2D幾何計算・AABB）
//--------------------------------------

// クロス積（2D）
static float Cross(const Vector2& a, const Vector2& b) { return a.x * b.y - a.y * b.x; }

// 点が三角形の内部にあるか判定
static bool IsPointInTriangle(const Vector2& p, const Vector2& a, const Vector2& b, const Vector2& c)
{
    float cp1 = Cross({ b.x - a.x, b.y - a.y }, { p.x - a.x, p.y - a.y });
    float cp2 = Cross({ c.x - b.x, c.y - b.y }, { p.x - b.x, p.y - b.y });
    float cp3 = Cross({ a.x - c.x, a.y - c.y }, { p.x - c.x, p.y - c.y });
    // 全て同じ符号なら内部（辺上含む）
    return (cp1 >= -EPSILON && cp2 >= -EPSILON && cp3 >= -EPSILON) || (cp1 <= EPSILON && cp2 <= EPSILON && cp3 <= EPSILON);
}

// UV計算（従来の平面投影・タイリング用）
static XMFLOAT2 CalculatePlanarUV(const XMFLOAT3& pos, const XMVECTOR& planeNormal, const XMVECTOR& planeTangent, const XMVECTOR& planeBinormal)
{
    (void)planeNormal;

    XMVECTOR vPos = XMLoadFloat3(&pos);
    float u = XMVectorGetX(XMVector3Dot(vPos, planeTangent));
    float v = XMVectorGetX(XMVector3Dot(vPos, planeBinormal));
    return XMFLOAT2(u * 0.5f + 0.5f, v * 0.5f + 0.5f);
}

// 頂点のウェルディング（座標統合）
static int GetWeldedVertexIndex(std::vector<XMFLOAT3>& vertices, const XMFLOAT3& pos)
{
    for (int i = 0; i < (int)vertices.size(); ++i)
    {
        const XMFLOAT3& v = vertices[i];
        if (std::abs(v.x - pos.x) < EPSILON && std::abs(v.y - pos.y) < EPSILON && std::abs(v.z - pos.z) < EPSILON)
        {
            return i;
        }
    }
    vertices.push_back(pos);
    return (int)vertices.size() - 1;
}

// AABBと平面の判定
static PlaneSide CheckAABBPlane(const AABB& aabb, const XMVECTOR& planeEq)
{
    // AABBの8頂点を作成
    XMFLOAT3 corners[8] = {
        { aabb.min.x, aabb.min.y, aabb.min.z },
        { aabb.min.x, aabb.min.y, aabb.max.z },
        { aabb.min.x, aabb.max.y, aabb.min.z },
        { aabb.min.x, aabb.max.y, aabb.max.z },
        { aabb.max.x, aabb.min.y, aabb.min.z },
        { aabb.max.x, aabb.min.y, aabb.max.z },
        { aabb.max.x, aabb.max.y, aabb.min.z },
        { aabb.max.x, aabb.max.y, aabb.max.z }
    };

    int frontCount = 0;
    int backCount = 0;

    for (int i = 0; i < 8; ++i)
    {
        float dist = XMVectorGetX(XMPlaneDotCoord(planeEq, XMLoadFloat3(&corners[i])));
        if (dist > EPSILON) frontCount++;
        else if (dist < -EPSILON) backCount++;
        else {
            // 平面上（許容範囲内）の場合は両方にカウントまたは無視
            // ここでは厳密判定のため無視し、交差扱いになりやすくする
        }
    }

    if (backCount == 0) return PlaneSide::Front; // 全て表
    if (frontCount == 0) return PlaneSide::Back; // 全て裏
    return PlaneSide::Intersect;                 // 交差
}

// メッシュデータからAABBを計算する
static AABB CalculateMeshAABB(const MeshData& mesh)
{
    AABB aabb;
    aabb.min = { FLT_MAX, FLT_MAX, FLT_MAX };
    aabb.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    if (mesh.vertices.empty()) return aabb;

    for (const auto& v : mesh.vertices)
    {
        aabb.min.x = std::min(aabb.min.x, v.position.x);
        aabb.min.y = std::min(aabb.min.y, v.position.y);
        aabb.min.z = std::min(aabb.min.z, v.position.z);

        aabb.max.x = std::max(aabb.max.x, v.position.x);
        aabb.max.y = std::max(aabb.max.y, v.position.y);
        aabb.max.z = std::max(aabb.max.z, v.position.z);
    }
    return aabb;
}

//--------------------------------------
// ループ抽出 & 多角形分割（Ear Clipping）
//--------------------------------------

/**
 * @brief エッジスープから閉じたループを抽出する
 */
static void ExtractLoops(const std::vector<RawEdge>& rawEdges, std::vector<std::vector<XMFLOAT3>>& outLoops)
{
    if (rawEdges.empty())
        return;

    std::vector<XMFLOAT3> uniqueVertices;
    std::map<int, std::vector<int>> adjacency;

    // 1. ウェルディングとグラフ構築
    for (const auto& e : rawEdges)
    {
        int s = GetWeldedVertexIndex(uniqueVertices, e.start);
        int eIdx = GetWeldedVertexIndex(uniqueVertices, e.end);
        if (s != eIdx)
        {
            adjacency[s].push_back(eIdx);
        }
    }

    // 2. ループ追跡
    while (!adjacency.empty())
    {
        std::vector<XMFLOAT3> currentLoop;
        auto itStart = adjacency.begin();
        int startNode = itStart->first;
        int currentNode = startNode;
        bool loopClosed = false;

        while (true)
        {
            currentLoop.push_back(uniqueVertices[currentNode]);

            auto it = adjacency.find(currentNode);
            if (it == adjacency.end() || it->second.empty())
                break; // 経路途切れ

            int nextNode = it->second.back();
            it->second.pop_back();
            if (it->second.empty())
                adjacency.erase(it);

            currentNode = nextNode;
            if (currentNode == startNode)
            {
                loopClosed = true;
                break;
            }
        }

        if (loopClosed && currentLoop.size() >= 3)
        {
            outLoops.push_back(currentLoop);
        }
    }
}

/**
 * @brief 耳刈り取り法による三角形分割
 */
static void TriangulateEarClipping(const std::vector<XMFLOAT3>& loop3D, const XMVECTOR& planeNormal, std::vector<unsigned int>& outIndices, int baseIndexOffset)
{
    int count = (int)loop3D.size();
    if (count < 3)
        return;

    // 1. 2D平面へ投影するための基底ベクトル
    XMVECTOR vUp = (std::abs(XMVectorGetY(planeNormal)) > 0.9f) ? XMVectorSet(0, 0, 1, 0) : XMVectorSet(0, 1, 0, 0);
    XMVECTOR vTangent = XMVector3Normalize(XMVector3Cross(planeNormal, vUp));
    XMVECTOR vBinormal = XMVector3Normalize(XMVector3Cross(planeNormal, vTangent));

    std::vector<Vector2> poly(count);
    std::vector<int> indices(count); // 頂点のインデックスリスト（これを削っていく）

    // 2D投影
    for (int i = 0; i < count; ++i)
    {
        XMVECTOR pos = XMLoadFloat3(&loop3D[i]);
        poly[i].x = XMVectorGetX(XMVector3Dot(pos, vTangent));
        poly[i].y = XMVectorGetX(XMVector3Dot(pos, vBinormal));
        indices[i] = i;
    }

    // 2. 巻き順の確認（符号付き面積）
    float area = 0;
    for (int i = 0; i < count; ++i)
    {
        Vector2 p1 = poly[i];
        Vector2 p2 = poly[(i + 1) % count];
        area += (p1.x * p2.y - p2.x * p1.y);
    }

    // Ear ClippingはCCW（反時計回り）であることを前提とする
    // 面積が負なら時計回りなので、インデックス順序を反転
    if (area < 0)
    {
        std::reverse(indices.begin(), indices.end());
    }

    // 3. 耳刈り取りループ
    // リストから頂点を取り除きながら三角形を作る
    int safetyCount = count * count; // 無限ループ防止

    while (indices.size() > 2 && safetyCount > 0)
    {
        safetyCount--;
        bool earFound = false;
        int n = (int)indices.size();

        for (int i = 0; i < n; ++i)
        {
            int prevIdx = indices[(i + n - 1) % n];
            int currIdx = indices[i];
            int nextIdx = indices[(i + 1) % n];

            const Vector2& a = poly[prevIdx];
            const Vector2& b = poly[currIdx];
            const Vector2& c = poly[nextIdx];

            // 凸性判定（Cross積が正なら凸、負なら凹）
            // CCWなら左折が正
            float cp = Cross({ b.x - a.x, b.y - a.y }, { c.x - b.x, c.y - b.y });
            if (cp <= EPSILON)
                continue; // 凹頂点または一直線

            // 他の頂点がこの三角形に含まれていないか確認
            bool isEar = true;
            for (int j = 0; j < n; ++j)
            {
                int checkIdx = indices[j];
                if (checkIdx == prevIdx || checkIdx == currIdx || checkIdx == nextIdx)
                    continue;

                if (IsPointInTriangle(poly[checkIdx], a, b, c))
                {
                    isEar = false;
                    break;
                }
            }

            if (isEar)
            {
                // 三角形生成（元のメッシュでのインデックスを保存）
                outIndices.push_back(baseIndexOffset + prevIdx);
                outIndices.push_back(baseIndexOffset + currIdx);
                outIndices.push_back(baseIndexOffset + nextIdx);

                // 耳（現在の頂点）をリストから削除
                indices.erase(indices.begin() + i);
                earFound = true;
                break;
            }
        }

        if (!earFound)
        {
            // 万が一耳が見つからない場合（自己交差など）、強制的に最初の3つを結んで進める
            outIndices.push_back(baseIndexOffset + indices[0]);
            outIndices.push_back(baseIndexOffset + indices[1]);
            outIndices.push_back(baseIndexOffset + indices[2]);
            indices.erase(indices.begin() + 1);
        }
    }
}

/**
 * @brief 断面メッシュ生成（Ear Clipping版）
 * @detail テクスチャIDが指定されている場合、断面全体のバウンディングボックスに合わせてUVを0-1にマッピングします
 */
static void CreateCapMesh(int texId, MeshData& mesh, const std::vector<RawEdge>& rawEdges, const XMVECTOR& planeNormal)
{
    std::vector<std::vector<XMFLOAT3>> loops;
    ExtractLoops(rawEdges, loops); // ループ抽出

    if (loops.empty())
        return;

    // 基底ベクトル計算（投影用）
    XMVECTOR vUp = (std::abs(XMVectorGetY(planeNormal)) > 0.9f) ? XMVectorSet(0, 0, 1, 0) : XMVectorSet(0, 1, 0, 0);
    XMVECTOR vTangent = XMVector3Normalize(XMVector3Cross(planeNormal, vUp));
    XMVECTOR vBinormal = XMVector3Normalize(XMVector3Cross(planeNormal, vTangent));

    for (const auto& loop : loops)
    {
        // 頂点登録開始位置
        int baseIndex = (int)mesh.vertices.size();

        // ----------------------------------------------------
        // UV計算のための前処理：ループ全体のバウンディングボックスを計算
        // ----------------------------------------------------
        float minU = FLT_MAX, maxU = -FLT_MAX;
        float minV = FLT_MAX, maxV = -FLT_MAX;

        // もしテクスチャ指定があれば、バウンディングボックスを計算する
        if (texId != -1)
        {
            for (const auto& pos : loop)
            {
                XMVECTOR vPos = XMLoadFloat3(&pos);
                float u = XMVectorGetX(XMVector3Dot(vPos, vTangent));
                float v = XMVectorGetX(XMVector3Dot(vPos, vBinormal));

                if (u < minU) minU = u;
                if (u > maxU) maxU = u;
                if (v < minV) minV = v;
                if (v > maxV) maxV = v;
            }

            // サイズが小さすぎる場合のゼロ除算対策
            if (std::abs(maxU - minU) < EPSILON) maxU = minU + 1.0f;
            if (std::abs(maxV - minV) < EPSILON) maxV = minV + 1.0f;
        }

        // ----------------------------------------------------
        // 頂点生成
        // ----------------------------------------------------
        for (const auto& pos : loop)
        {
            Vertex v;
            v.position = pos;
            XMStoreFloat3(&v.normal, planeNormal);
            v.color = { 1, 1, 1, 1 };

            if (texId != -1)
            {
                // バウンディングボックスに合わせてUVを正規化 (0.0 ～ 1.0)
                // これにより、断面の形状いっぱいにテクスチャがフィットします
                XMVECTOR vPos = XMLoadFloat3(&pos);
                float rawU = XMVectorGetX(XMVector3Dot(vPos, vTangent));
                float rawV = XMVectorGetX(XMVector3Dot(vPos, vBinormal));

                v.uv.x = (rawU - minU) / (maxU - minU);
                v.uv.y = (rawV - minV) / (maxV - minV);
            }
            else
            {
                // テクスチャなし（またはデフォルト）の場合は従来の平面投影
                v.uv = CalculatePlanarUV(pos, planeNormal, vTangent, vBinormal);
            }
            mesh.vertices.push_back(v);
        }

        // Ear Clippingでインデックス生成
        TriangulateEarClipping(loop, planeNormal, mesh.indices, baseIndex);
    }
}

//======================================
// クラス実装部
//======================================

bool Slicer::Slice(MODEL* targetModel, const XMFLOAT4X4& worldMatrix, const XMFLOAT3& planePoint, const XMFLOAT3& planeNormal, MODEL** outFront, MODEL** outBack)
{
    if (!targetModel)
        return false;

    // 1. 座標変換
    XMMATRIX mWorld = XMLoadFloat4x4(&worldMatrix);
    XMMATRIX mInvWorld = XMMatrixInverse(nullptr, mWorld);
    XMVECTOR vLocalPoint = XMVector3TransformCoord(XMLoadFloat3(&planePoint), mInvWorld);
    XMVECTOR vLocalNormal = XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&planeNormal), mInvWorld));
    float distanceD = -XMVectorGetX(XMVector3Dot(vLocalNormal, vLocalPoint));
    XMVECTOR planeEq = XMVectorSetW(vLocalNormal, distanceD);

    // ---------------------------------------------------------
    // 【最適化】モデル全体のAABBチェック
    // ---------------------------------------------------------
    // モデルのローカルAABBと切断平面を比較する
    PlaneSide modelSide = CheckAABBPlane(targetModel->local_aabb, planeEq);

    if (modelSide == PlaneSide::Front)
    {
        // 完全に表側 -> 切断なし（Frontのみ、Backなし）
        // 呼び出し元で「false」として扱うことで、無駄なメモリ確保とコピーを防ぐ
        *outFront = nullptr;
        *outBack = nullptr;
        return false;
    }
    else if (modelSide == PlaneSide::Back)
    {
        // 完全に裏側 -> 切断なし（Backのみ、Frontなし）
        *outFront = nullptr;
        *outBack = nullptr;
        return false;
    }

    // ---------------------------------------------------------
    // ここから実際の切断処理（交差している場合のみ到達）
    // ---------------------------------------------------------

    int capMaterialIndex = 0;
    if (targetModel->pSliceTextureSRV != nullptr)
    {
        // 断面用テクスチャがある場合、既存リストの「次」に追加されるはずなので、そのインデックスを指定
        capMaterialIndex = (int)targetModel->materials.size();
    }
    else
    {
        // テクスチャがない場合は、元のモデルの素材（0番など）を使い回す
        if (!targetModel->Meshes.empty())
            capMaterialIndex = targetModel->Meshes[0].materialIndex;
    }

    // 2. メッシュ分割 & エッジ収集
    std::vector<MeshData> frontMeshes, backMeshes;
    std::vector<RawEdge> allFrontCapEdges, allBackCapEdges;

    int fallbackMatIndex = 0;
    if (!targetModel->Meshes.empty())
        fallbackMatIndex = targetModel->Meshes[0].materialIndex;

    for (const auto& srcMesh : targetModel->Meshes)
    {
        // ---------------------------------------------------------
        // 【最適化】メッシュ単位のAABBチェック
        // ---------------------------------------------------------
        // 個別のメッシュが平面に対してどう配置されているか確認
        // AABBを都度計算するコスト(O(V))はあるが、Slicing(O(V) + α)よりは遥かに軽い
        AABB meshAABB = CalculateMeshAABB(srcMesh);
        PlaneSide meshSide = CheckAABBPlane(meshAABB, planeEq);

        if (meshSide == PlaneSide::Front)
        {
            // 丸ごとFrontへコピー
            frontMeshes.push_back(srcMesh);
            continue; // 次のメッシュへ
        }
        else if (meshSide == PlaneSide::Back)
        {
            // 丸ごとBackへコピー
            backMeshes.push_back(srcMesh);
            continue; // 次のメッシュへ
        }

        // 交差している場合のみ、三角形ごとの詳細チェックを行う
        MeshData frontMesh, backMesh;
        frontMesh.materialIndex = srcMesh.materialIndex;
        backMesh.materialIndex = srcMesh.materialIndex;

        const auto& verts = srcMesh.vertices;
        const auto& inds = srcMesh.indices;

        for (size_t i = 0; i < inds.size(); i += 3)
        {
            if (inds[i + 2] >= verts.size())
                continue;

            Vertex v[3] = { verts[inds[i]], verts[inds[i + 1]], verts[inds[i + 2]] };
            float d[3];
            bool bFront[3];

            for (int j = 0; j < 3; ++j)
            {
                d[j] = GetDistanceToPlane(v[j].position, planeEq);
                bFront[j] = (d[j] >= 0);
            }

            if (bFront[0] == bFront[1] && bFront[1] == bFront[2])
            {
                MeshData* dest = bFront[0] ? &frontMesh : &backMesh;
                unsigned int base = (unsigned int)dest->vertices.size();
                for (int j = 0; j < 3; ++j)
                    dest->vertices.push_back(v[j]);
                dest->indices.push_back(base);
                dest->indices.push_back(base + 1);
                dest->indices.push_back(base + 2);
                continue;
            }

            int isoIdx = -1;
            if (bFront[0] == bFront[2])
                isoIdx = 1;
            else if (bFront[1] == bFront[2])
                isoIdx = 0;
            else
                isoIdx = 2;

            Vertex tri[3] = { v[isoIdx], v[(isoIdx + 1) % 3], v[(isoIdx + 2) % 3] };
            float dist[3] = { d[isoIdx], d[(isoIdx + 1) % 3], d[(isoIdx + 2) % 3] };

            float t0 = dist[0] / (dist[0] - dist[1]);
            float t1 = dist[0] / (dist[0] - dist[2]);

            Vertex split0 = Interpolate(tri[0], tri[1], t0);
            Vertex split1 = Interpolate(tri[0], tri[2], t1);

            if (bFront[isoIdx])
            {
                // Front側が孤立 -> 断面は split0 -> split1
                allFrontCapEdges.push_back({ split0.position, split1.position });
                allBackCapEdges.push_back({ split1.position, split0.position });
            }
            else
            {
                // Back側が孤立 -> 断面は split0 -> split1
                allBackCapEdges.push_back({ split0.position, split1.position });
                allFrontCapEdges.push_back({ split1.position, split0.position });
            }

            MeshData* isoDest = bFront[isoIdx] ? &frontMesh : &backMesh;
            MeshData* majDest = bFront[isoIdx] ? &backMesh : &frontMesh;

            unsigned int baseIso = (unsigned int)isoDest->vertices.size();
            isoDest->vertices.push_back(tri[0]);
            isoDest->vertices.push_back(split0);
            isoDest->vertices.push_back(split1);
            isoDest->indices.push_back(baseIso);
            isoDest->indices.push_back(baseIso + 1);
            isoDest->indices.push_back(baseIso + 2);

            unsigned int baseMaj = (unsigned int)majDest->vertices.size();
            majDest->vertices.push_back(tri[1]);
            majDest->vertices.push_back(tri[2]);
            majDest->vertices.push_back(split1);
            majDest->vertices.push_back(split0);
            majDest->indices.push_back(baseMaj);
            majDest->indices.push_back(baseMaj + 1);
            majDest->indices.push_back(baseMaj + 2);
            majDest->indices.push_back(baseMaj);
            majDest->indices.push_back(baseMaj + 2);
            majDest->indices.push_back(baseMaj + 3);
        }

        if (!frontMesh.vertices.empty())
            frontMeshes.push_back(frontMesh);
        if (!backMesh.vertices.empty())
            backMeshes.push_back(backMesh);
    }

    // 3. 断面生成（Ear Clipping適用）
    if (!allFrontCapEdges.empty())
    {
        MeshData capMesh;
        capMesh.materialIndex = capMaterialIndex;
        // もし別途マテリアルを用意するなら、ここでtexIdに対応するマテリアルIDを設定する
        CreateCapMesh(targetModel->GetSlicedTexturId(), capMesh, allFrontCapEdges, -vLocalNormal); // Frontは逆法線
        if (!capMesh.vertices.empty())
            frontMeshes.push_back(capMesh);
    }

    if (!allBackCapEdges.empty())
    {
        MeshData capMesh;
        capMesh.materialIndex = capMaterialIndex;
        CreateCapMesh(targetModel->GetSlicedTexturId(), capMesh, allBackCapEdges, vLocalNormal);
        if (!capMesh.vertices.empty())
            backMeshes.push_back(capMesh);
    }

    // 両方のメッシュが存在する場合のみ「切断成功」とする
    if (frontMeshes.empty() || backMeshes.empty())
    {
        *outFront = nullptr;
        *outBack = nullptr;
        return false;
    }

    *outFront = ModelCreateFromData(frontMeshes, targetModel);
    *outBack = ModelCreateFromData(backMeshes, targetModel);

    return true;
}

/*
*@brief CPU専用スライス処理（GPUバッファ作成なし）
*/
bool Slicer::SliceCPUOnly(
    MODEL * targetModel,
    const XMFLOAT4X4 & worldMatrix,
    const XMFLOAT3 & planePoint,
    const XMFLOAT3 & planeNormal,
    std::vector<MeshData>&outFrontMeshes,
    std::vector<MeshData>&outBackMeshes)
{
    if (!targetModel)
        return false;

    // 座標変換
    XMMATRIX mWorld = XMLoadFloat4x4(&worldMatrix);
    XMMATRIX mInvWorld = XMMatrixInverse(nullptr, mWorld);
    XMVECTOR vLocalPoint = XMVector3TransformCoord(XMLoadFloat3(&planePoint), mInvWorld);
    XMVECTOR vLocalNormal = XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&planeNormal), mInvWorld));
    float distanceD = -XMVectorGetX(XMVector3Dot(vLocalNormal, vLocalPoint));
    XMVECTOR planeEq = XMVectorSetW(vLocalNormal, distanceD);

    // モデル全体のAABBチェック（最適化）
    PlaneSide modelSide = CheckAABBPlane(targetModel->local_aabb, planeEq);
    if (modelSide == PlaneSide::Front || modelSide == PlaneSide::Back)
    {
        return false;
    }

    // 断面用マテリアルインデックス
    int capMaterialIndex = 0;
    if (targetModel->pSliceTextureSRV != nullptr)
    {
        capMaterialIndex = (int)targetModel->materials.size();
    }
    else
    {
        if (!targetModel->Meshes.empty())
            capMaterialIndex = targetModel->Meshes[0].materialIndex;
    }

    // メッシュ分割処理
    std::vector<RawEdge> allFrontCapEdges, allBackCapEdges;

    for (const auto& srcMesh : targetModel->Meshes)
    {
        AABB meshAABB = CalculateMeshAABB(srcMesh);
        PlaneSide meshSide = CheckAABBPlane(meshAABB, planeEq);

        if (meshSide == PlaneSide::Front)
        {
            outFrontMeshes.push_back(srcMesh);
            continue;
        }
        else if (meshSide == PlaneSide::Back)
        {
            outBackMeshes.push_back(srcMesh);
            continue;
        }

        // 交差メッシュの処理（既存コードと同じロジック）
        MeshData frontMesh, backMesh;
        frontMesh.materialIndex = srcMesh.materialIndex;
        backMesh.materialIndex = srcMesh.materialIndex;

        const auto& verts = srcMesh.vertices;
        const auto& inds = srcMesh.indices;

        for (size_t i = 0; i < inds.size(); i += 3)
        {
            if (inds[i + 2] >= verts.size())
                continue;

            Vertex v[3] = { verts[inds[i]], verts[inds[i + 1]], verts[inds[i + 2]] };
            float d[3];
            bool bFront[3];

            for (int j = 0; j < 3; ++j)
            {
                d[j] = GetDistanceToPlane(v[j].position, planeEq);
                bFront[j] = (d[j] >= 0);
            }

            if (bFront[0] == bFront[1] && bFront[1] == bFront[2])
            {
                MeshData* dest = bFront[0] ? &frontMesh : &backMesh;
                unsigned int base = (unsigned int)dest->vertices.size();
                for (int j = 0; j < 3; ++j)
                    dest->vertices.push_back(v[j]);
                dest->indices.push_back(base);
                dest->indices.push_back(base + 1);
                dest->indices.push_back(base + 2);
                continue;
            }

            // 三角形分割処理（既存ロジックと同じ）
            int isoIdx = -1;
            if (bFront[0] == bFront[2])
                isoIdx = 1;
            else if (bFront[1] == bFront[2])
                isoIdx = 0;
            else
                isoIdx = 2;

            Vertex tri[3] = { v[isoIdx], v[(isoIdx + 1) % 3], v[(isoIdx + 2) % 3] };
            float dist[3] = { d[isoIdx], d[(isoIdx + 1) % 3], d[(isoIdx + 2) % 3] };

            float t0 = dist[0] / (dist[0] - dist[1]);
            float t1 = dist[0] / (dist[0] - dist[2]);

            Vertex split0 = Interpolate(tri[0], tri[1], t0);
            Vertex split1 = Interpolate(tri[0], tri[2], t1);

            if (bFront[isoIdx])
            {
                allFrontCapEdges.push_back({ split0.position, split1.position });
                allBackCapEdges.push_back({ split1.position, split0.position });
            }
            else
            {
                allBackCapEdges.push_back({ split0.position, split1.position });
                allFrontCapEdges.push_back({ split1.position, split0.position });
            }

            MeshData* isoDest = bFront[isoIdx] ? &frontMesh : &backMesh;
            MeshData* majDest = bFront[isoIdx] ? &backMesh : &frontMesh;

            unsigned int baseIso = (unsigned int)isoDest->vertices.size();
            isoDest->vertices.push_back(tri[0]);
            isoDest->vertices.push_back(split0);
            isoDest->vertices.push_back(split1);
            isoDest->indices.push_back(baseIso);
            isoDest->indices.push_back(baseIso + 1);
            isoDest->indices.push_back(baseIso + 2);

            unsigned int baseMaj = (unsigned int)majDest->vertices.size();
            majDest->vertices.push_back(tri[1]);
            majDest->vertices.push_back(tri[2]);
            majDest->vertices.push_back(split1);
            majDest->vertices.push_back(split0);
            majDest->indices.push_back(baseMaj);
            majDest->indices.push_back(baseMaj + 1);
            majDest->indices.push_back(baseMaj + 2);
            majDest->indices.push_back(baseMaj);
            majDest->indices.push_back(baseMaj + 2);
            majDest->indices.push_back(baseMaj + 3);
        }

        if (!frontMesh.vertices.empty())
            outFrontMeshes.push_back(frontMesh);
        if (!backMesh.vertices.empty())
            outBackMeshes.push_back(backMesh);
    }

    // 断面生成
    if (!allFrontCapEdges.empty())
    {
        MeshData capMesh;
        capMesh.materialIndex = capMaterialIndex;
        CreateCapMesh(targetModel->GetSlicedTexturId(), capMesh, allFrontCapEdges, -vLocalNormal);
        if (!capMesh.vertices.empty())
            outFrontMeshes.push_back(capMesh);
    }

    if (!allBackCapEdges.empty())
    {
        MeshData capMesh;
        capMesh.materialIndex = capMaterialIndex;
        CreateCapMesh(targetModel->GetSlicedTexturId(), capMesh, allBackCapEdges, vLocalNormal);
        if (!capMesh.vertices.empty())
            outBackMeshes.push_back(capMesh);
    }

    return !outFrontMeshes.empty() && !outBackMeshes.empty();
}

Vertex Slicer::Interpolate(const Vertex& v1, const Vertex& v2, float t)
{
    Vertex out;
    XMVECTOR p1 = XMLoadFloat3(&v1.position);
    XMVECTOR p2 = XMLoadFloat3(&v2.position);
    XMVECTOR n1 = XMLoadFloat3(&v1.normal);
    XMVECTOR n2 = XMLoadFloat3(&v2.normal);
    XMVECTOR c1 = XMLoadFloat4(&v1.color);
    XMVECTOR c2 = XMLoadFloat4(&v2.color);
    XMVECTOR uv1 = XMLoadFloat2(&v1.uv);
    XMVECTOR uv2 = XMLoadFloat2(&v2.uv);

    XMStoreFloat3(&out.position, XMVectorLerp(p1, p2, t));
    XMStoreFloat3(&out.normal, XMVector3Normalize(XMVectorLerp(n1, n2, t)));
    XMStoreFloat4(&out.color, XMVectorLerp(c1, c2, t));
    XMStoreFloat2(&out.uv, XMVectorLerp(uv1, uv2, t));
    return out;
}

float Slicer::GetDistanceToPlane(const XMFLOAT3& vertex, const XMVECTOR& planeVector) { return XMVectorGetX(XMPlaneDotCoord(planeVector, XMLoadFloat3(&vertex))); }