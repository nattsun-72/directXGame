/****************************************
 * @file slice_task_manager.cpp
 * @brief 非同期スライス処理の実装
 * @author Natsume Shidara
 * @date 2025/01/05
 * @update 2026/01/13 - planeNormalをSliceResultに引き継ぎ
 ****************************************/

#include "slice_task_manager.h"
#include "debug_ostream.h"
#include <algorithm>

using namespace DirectX;

//--------------------------------------
// 静的メンバ変数の定義
//--------------------------------------
std::vector<std::thread> SliceTaskManager::s_WorkerThreads;
std::queue<SliceRequest> SliceTaskManager::s_RequestQueue;
std::queue<SliceResult> SliceTaskManager::s_ResultQueue;
std::mutex SliceTaskManager::s_RequestMutex;
std::mutex SliceTaskManager::s_ResultMutex;
std::condition_variable SliceTaskManager::s_RequestCondition;
std::atomic<bool> SliceTaskManager::s_ShouldTerminate(false);
std::atomic<int> SliceTaskManager::s_NextRequestId(1);
std::atomic<int> SliceTaskManager::s_PendingTaskCount(0);

//======================================
// 初期化・終了
//======================================

void SliceTaskManager::Initialize(int workerThreadCount)
{
    s_ShouldTerminate = false;
    s_NextRequestId = 1;
    s_PendingTaskCount = 0;

    // ワーカースレッドを起動
    for (int i = 0; i < workerThreadCount; ++i)
    {
        s_WorkerThreads.emplace_back(WorkerThreadFunction);
    }

    OutputDebugStringA("[SliceTaskManager] Initialized with ");
    OutputDebugStringA(std::to_string(workerThreadCount).c_str());
    OutputDebugStringA(" worker threads\n");
}

void SliceTaskManager::Finalize()
{
    // 終了フラグを立てる
    s_ShouldTerminate = true;
    s_RequestCondition.notify_all();

    // 全スレッドの終了を待機
    for (auto& thread : s_WorkerThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
    s_WorkerThreads.clear();

    // 未処理リクエストのクリーンアップ
    {
        std::lock_guard<std::mutex> lock(s_RequestMutex);
        while (!s_RequestQueue.empty())
        {
            SliceRequest& req = s_RequestQueue.front();
            ModelRelease(req.targetModel);
            s_RequestQueue.pop();
        }
    }

    // 未取得の結果のクリーンアップ
    {
        std::lock_guard<std::mutex> lock(s_ResultMutex);
        while (!s_ResultQueue.empty())
        {
            SliceResult& result = s_ResultQueue.front();
            ModelRelease(result.originalModel);
            s_ResultQueue.pop();
        }
    }

    OutputDebugStringA("[SliceTaskManager] Finalized\n");
}

//======================================
// リクエスト送信
//======================================

int SliceTaskManager::EnqueueSlice(const SliceRequest& request)
{
    int requestId = s_NextRequestId++;

    SliceRequest req = request;
    req.requestId = requestId;

    // モデルの参照カウントを増加（ワーカースレッドが使用するため）
    ModelAddRef(req.targetModel);

    {
        std::lock_guard<std::mutex> lock(s_RequestMutex);
        s_RequestQueue.push(req);
    }

    s_PendingTaskCount++;
    s_RequestCondition.notify_one();

    return requestId;
}

//======================================
// 結果取得
//======================================

bool SliceTaskManager::TryGetCompletedResult(SliceResult& outResult)
{
    std::lock_guard<std::mutex> lock(s_ResultMutex);

    if (s_ResultQueue.empty())
    {
        return false;
    }

    outResult = std::move(s_ResultQueue.front());
    s_ResultQueue.pop();
    s_PendingTaskCount--;

    return true;
}

//======================================
// 状態取得
//======================================

int SliceTaskManager::GetPendingTaskCount()
{
    return s_PendingTaskCount.load();
}

//======================================
// ワーカースレッド処理
//======================================

void SliceTaskManager::WorkerThreadFunction()
{
    while (!s_ShouldTerminate)
    {
        SliceRequest request;

        // リクエストキューから取得
        {
            std::unique_lock<std::mutex> lock(s_RequestMutex);

            // リクエストが来るまで待機
            s_RequestCondition.wait(lock, []() {
                return !s_RequestQueue.empty() || s_ShouldTerminate;
                                    });

            if (s_ShouldTerminate)
            {
                break;
            }

            if (s_RequestQueue.empty())
            {
                continue;
            }

            request = s_RequestQueue.front();
            s_RequestQueue.pop();
        }

        // スライス処理実行（CPUのみ、GPUリソース生成なし）
        SliceResult result;
        result.requestId = request.requestId;
        result.originalModel = request.targetModel;
        result.originalPosition = request.originalPosition;
        result.originalVelocity = request.originalVelocity;
        result.originalMass = request.originalMass;
        result.rootVolume = request.rootVolume;
        result.colliderType = request.colliderType;
        result.planeNormal = request.planeNormal;  // 切断平面法線を引き継ぎ

        // Slicer::SliceをCPUデータのみ取得する版で呼び出す
        result.success = Slicer::SliceCPUOnly(
            request.targetModel,
            request.worldMatrix,
            request.planePoint,
            request.planeNormal,
            result.frontMeshes,
            result.backMeshes
        );

        // 結果をキューに追加
        {
            std::lock_guard<std::mutex> lock(s_ResultMutex);
            s_ResultQueue.push(std::move(result));
        }
    }
}