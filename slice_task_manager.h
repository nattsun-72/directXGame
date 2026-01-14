/****************************************
 * @file slice_task_manager.h
 * @brief 非同期スライス処理タスク管理
 * @author Natsume Shidara
 * @date 2025/01/05
 * @update 2026/01/13 - SliceResultにplaneNormal追加
 ****************************************/
#pragma once
#include "model.h"
#include "slicer.h"
#include "collider.h"
#include <DirectXMath.h>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

 //--------------------------------------
 // スライスリクエスト構造体
 //--------------------------------------
struct SliceRequest
{
    MODEL* targetModel;              // 切断対象モデル（参照カウント済み）
    DirectX::XMFLOAT4X4 worldMatrix; // ワールド変換行列
    DirectX::XMFLOAT3 planePoint;    // 平面上の点
    DirectX::XMFLOAT3 planeNormal;   // 平面法線

    // 元オブジェクトの物理情報
    DirectX::XMFLOAT3 originalPosition;
    DirectX::XMFLOAT3 originalVelocity;
    float originalMass;
    float rootVolume;
    ColliderType colliderType;

    int requestId; // リクエスト識別用ID
};

//--------------------------------------
// スライス結果構造体（CPUデータのみ）
//--------------------------------------
struct SliceResult
{
    int requestId;
    bool success;

    // CPU側のメッシュデータ（GPUバッファ未作成）
    std::vector<MeshData> frontMeshes;
    std::vector<MeshData> backMeshes;

    // 元モデルの参照（マテリアル情報継承用）
    MODEL* originalModel;

    // 物理情報の引き継ぎ
    DirectX::XMFLOAT3 originalPosition;
    DirectX::XMFLOAT3 originalVelocity;
    float originalMass;
    float rootVolume;
    ColliderType colliderType;

    // 切断平面法線（分離方向計算用）
    DirectX::XMFLOAT3 planeNormal;
};

//--------------------------------------
// 非同期スライスタスクマネージャー
//--------------------------------------
class SliceTaskManager
{
public:
    static void Initialize(int workerThreadCount = 4);
    static void Finalize();

    // スライスリクエストをキューに追加（メインスレッド）
    static int EnqueueSlice(const SliceRequest& request);

    // 完了した結果を取得（メインスレッド）
    static bool TryGetCompletedResult(SliceResult& outResult);

    // 処理中のタスク数を取得
    static int GetPendingTaskCount();

private:
    // ワーカースレッドのエントリーポイント
    static void WorkerThreadFunction();

    // 内部データ
    static std::vector<std::thread> s_WorkerThreads;
    static std::queue<SliceRequest> s_RequestQueue;
    static std::queue<SliceResult> s_ResultQueue;
    static std::mutex s_RequestMutex;
    static std::mutex s_ResultMutex;
    static std::condition_variable s_RequestCondition;
    static std::atomic<bool> s_ShouldTerminate;
    static std::atomic<int> s_NextRequestId;
    static std::atomic<int> s_PendingTaskCount;
};