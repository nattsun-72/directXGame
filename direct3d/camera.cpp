/****************************************
 * @file camera.cpp
 * @brief デバッグ用カメラ制御モジュール
 * @detail
 * - FPSカメラ的な移動・回転操作
 * - 視点行列 / 投影行列生成
 * - デバッグ用カメラ情報表示
 * @author Natsume Shidara
 * @update 2025/12/19 (キャスト明示化対応)
 * @update 2026/01/10 - Bladeデバッグ情報を分離
 * @update 2026/01/13 - Releaseビルドクラッシュ修正
 ****************************************/

#include "camera.h"

#include <DirectXMath.h>
#include <iomanip>
#include <memory>
#include <sstream>

#include "color.h"
#include "direct3d.h"
#include "key_logger.h"

#ifdef _DEBUG
#include "debug_text.h"
#endif

using namespace DirectX;

//--------------------------------------
// グローバル変数群（カメラ状態）
//--------------------------------------
static XMFLOAT3 g_CameraPosition = { 0.0f, 0.0f, 0.0f }; // カメラ座標
static XMFLOAT3 g_CameraFront = { 0.0f, 0.0f, 1.0f };    // 前方向ベクトル
static XMFLOAT3 g_CameraUp = { 0.0f, 1.0f, 0.0f };       // 上方向ベクトル
static XMFLOAT3 g_CameraRight = { 1.0f, 0.0f, 0.0f };    // 右方向ベクトル

// 定数定義
constexpr float CAMERA_MOVE_SPEED = 7.5f;                          // 移動速度[m/s]
constexpr float CAMERA_ROTATION_SPEED = XMConvertToRadians(60.0f); // 回転速度[rad/s]

static XMFLOAT4X4 g_CameraViewMatrix;  // ビュー行列
static XMFLOAT4X4 g_PerspectiveMatrix; // 投影行列

static float g_Fov = XMConvertToRadians(60.0f); // 視野角（ラジアン）

//--------------------------------------
// システムリソース
//--------------------------------------
#ifdef _DEBUG
static std::unique_ptr<hal::DebugText> g_pDebugText = nullptr; // デバッグ描画用
#endif

static ID3D11Buffer* g_pVSConstantBuffer1 = nullptr; // View Matrix 用定数バッファ
static ID3D11Buffer* g_pVSConstantBuffer2 = nullptr; // Projection Matrix 用定数バッファ

static bool g_Initialized = false; // 初期化フラグ

//======================================
// 初期化 / 終了処理
//======================================

/**
 * @brief カメラ初期化（位置・前・上を指定）
 * @param position 初期位置
 * @param front    前方向ベクトル
 * @param up       上方向ベクトル
 */
void Camera_Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front, const DirectX::XMFLOAT3& up)
{
    // 基本初期化（共通の初期設定を呼び出し）
    Camera_Initialize();

    // ベクトルを読み込み
    XMVECTOR _front = XMLoadFloat3(&front);
    XMVECTOR _up = XMLoadFloat3(&up);

    // 正規化
    _front = XMVector3Normalize(_front);
    _up = XMVector3Normalize(_up);

    // 右ベクトル計算
    XMVECTOR _right = XMVector3Cross(_up, _front);
    _right = XMVector3Normalize(_right);

    // グローバルに保存
    XMStoreFloat3(&g_CameraFront, _front);
    XMStoreFloat3(&g_CameraUp, _up);
    XMStoreFloat3(&g_CameraRight, _right);

    g_CameraPosition = position;
}

/**
 * @brief カメラ基本初期化（既定値）
 * @detail リソースの確保と既定パラメータの設定を行う
 */
void Camera_Initialize()
{
    // 既に初期化済みの場合はスキップ（二重初期化防止）
    if (g_Initialized)
    {
        return;
    }

    g_CameraPosition = { 0.0f, 1.0f, -10.0f };
    g_CameraFront = { 0.0f, 0.0f, 1.0f };
    g_CameraUp = { 0.0f, 1.0f, 0.0f };
    g_CameraRight = { 1.0f, 0.0f, 0.0f };
    g_Fov = XMConvertToRadians(60.0f);

    // 行列初期化
    XMStoreFloat4x4(&g_CameraViewMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&g_PerspectiveMatrix, XMMatrixIdentity());

#ifdef _DEBUG
    g_pDebugText = std::make_unique<hal::DebugText>(
        Direct3D_GetDevice(),
        Direct3D_GetContext(),
        L"assets/consolab_ascii_512.png",
        static_cast<UINT>(Direct3D_GetBackBufferWidth()),
        static_cast<UINT>(Direct3D_GetBackBufferHeight()),
        0.0f,   // offsetX
        32.0f,  // offsetY
        0,      // maxLine (ULONG)
        0,      // maxCharactersPerLine (ULONG)
        0.0f,   // lineSpacing (float)
        16.0f   // characterSpacing (float)
    );
#endif

    // デバイス取得
    ID3D11Device* pDevice = Direct3D_GetDevice();
    if (pDevice == nullptr)
    {
        g_Initialized = false;
        return;
    }

    // 定数バッファ作成
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(XMFLOAT4X4);
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    HRESULT hr1 = pDevice->CreateBuffer(&bufferDesc, nullptr, &g_pVSConstantBuffer1);
    HRESULT hr2 = pDevice->CreateBuffer(&bufferDesc, nullptr, &g_pVSConstantBuffer2);

    if (FAILED(hr1) || FAILED(hr2))
    {
        // バッファ作成失敗時はクリーンアップ
        SAFE_RELEASE(g_pVSConstantBuffer1);
        SAFE_RELEASE(g_pVSConstantBuffer2);
        g_Initialized = false;
        return;
    }

    g_Initialized = true;
}

/**
 * @brief カメラ終了処理
 * @detail デバッグテキストと定数バッファを解放
 */
void Camera_Finalize()
{
#ifdef _DEBUG
    g_pDebugText.reset();
#endif

    SAFE_RELEASE(g_pVSConstantBuffer1);
    SAFE_RELEASE(g_pVSConstantBuffer2);

    g_Initialized = false;
}

//======================================
// カメラ更新処理
//======================================

/**
 * @brief カメラ更新処理
 * @param elapsed_time 前フレームからの経過時間（秒）
 * @detail キー入力による移動・回転、および行列の再計算を行う
 */
void Camera_Update(double elapsed_time)
{
    // doubleからfloatへ明示的にキャスト
    const float dt = static_cast<float>(elapsed_time);

    // 現在のベクトル読み込み
    XMVECTOR cameraPosition = XMLoadFloat3(&g_CameraPosition);
    XMVECTOR cameraFront = XMLoadFloat3(&g_CameraFront);
    XMVECTOR cameraUp = XMLoadFloat3(&g_CameraUp);
    XMVECTOR cameraRight = XMLoadFloat3(&g_CameraRight);

    //--------------------------------------
    // 移動処理（WASD + 上下）
    //--------------------------------------
    if (KeyLogger_IsPressed(KK_W))
    {
        XMVECTOR front = XMVector3Normalize(cameraFront * XMVECTOR{ 1.0f, 0.0f, 1.0f });
        cameraPosition += front * CAMERA_MOVE_SPEED * dt;
    }
    if (KeyLogger_IsPressed(KK_S))
    {
        XMVECTOR front = XMVector3Normalize(cameraFront * XMVECTOR{ 1.0f, 0.0f, 1.0f });
        cameraPosition -= front * CAMERA_MOVE_SPEED * dt;
    }
    if (KeyLogger_IsPressed(KK_A))
    {
        cameraPosition -= cameraRight * CAMERA_MOVE_SPEED * dt;
    }
    if (KeyLogger_IsPressed(KK_D))
    {
        cameraPosition += cameraRight * CAMERA_MOVE_SPEED * dt;
    }
    if (KeyLogger_IsPressed(KK_E))
    {
        cameraPosition += cameraUp * CAMERA_MOVE_SPEED * dt;
    }
    if (KeyLogger_IsPressed(KK_Q))
    {
        cameraPosition -= cameraUp * CAMERA_MOVE_SPEED * dt;
    }

    //--------------------------------------
    // FOV調整
    //--------------------------------------
    if (KeyLogger_IsPressed(KK_Z))
    {
        g_Fov -= XMConvertToRadians(10.0f) * dt;
    }
    if (KeyLogger_IsPressed(KK_C))
    {
        g_Fov += XMConvertToRadians(10.0f) * dt;
    }

    //--------------------------------------
    // 回転処理（矢印キー）
    //--------------------------------------
    // 上下: ピッチ
    if (KeyLogger_IsPressed(KK_DOWN))
    {
        XMMATRIX rotation = XMMatrixRotationAxis(cameraRight, CAMERA_ROTATION_SPEED * dt);
        cameraFront = XMVector3Normalize(XMVector3TransformNormal(cameraFront, rotation));
        cameraUp = XMVector3Normalize(XMVector3Cross(cameraFront, cameraRight));
    }
    if (KeyLogger_IsPressed(KK_UP))
    {
        XMMATRIX rotation = XMMatrixRotationAxis(cameraRight, -CAMERA_ROTATION_SPEED * dt);
        cameraFront = XMVector3Normalize(XMVector3TransformNormal(cameraFront, rotation));
        cameraUp = XMVector3Normalize(XMVector3Cross(cameraFront, cameraRight));
    }

    // 左右: ヨー
    if (KeyLogger_IsPressed(KK_LEFT))
    {
        XMMATRIX rotation = XMMatrixRotationY(-CAMERA_ROTATION_SPEED * dt);
        cameraUp = XMVector3Normalize(XMVector3TransformNormal(cameraUp, rotation));
        cameraFront = XMVector3Normalize(XMVector3TransformNormal(cameraFront, rotation));
        cameraRight = XMVector3Normalize(XMVector3Cross(cameraUp, cameraFront));
    }
    if (KeyLogger_IsPressed(KK_RIGHT))
    {
        XMMATRIX rotation = XMMatrixRotationY(CAMERA_ROTATION_SPEED * dt);
        cameraUp = XMVector3Normalize(XMVector3TransformNormal(cameraUp, rotation));
        cameraFront = XMVector3Normalize(XMVector3TransformNormal(cameraFront, rotation));
        cameraRight = XMVector3Normalize(XMVector3Cross(cameraUp, cameraFront));
    }

    // 更新したベクトルを格納
    XMStoreFloat3(&g_CameraPosition, cameraPosition);
    XMStoreFloat3(&g_CameraFront, cameraFront);
    XMStoreFloat3(&g_CameraUp, cameraUp);
    XMStoreFloat3(&g_CameraRight, cameraRight);

    //--------------------------------------
    // ビュー行列更新
    //--------------------------------------
    XMMATRIX mtxView = XMMatrixLookAtLH(cameraPosition, cameraPosition + cameraFront, cameraUp);
    XMStoreFloat4x4(&g_CameraViewMatrix, mtxView);


    //--------------------------------------
    // 投影行列更新
    //--------------------------------------
    float width = static_cast<float>(Direct3D_GetBackBufferWidth());
    float height = static_cast<float>(Direct3D_GetBackBufferHeight());
    float aspectRatio = width / height;
    float nearZ = 0.01f;
    float farZ = 10000.0f;

    XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(g_Fov, aspectRatio, nearZ, farZ);
    XMStoreFloat4x4(&g_PerspectiveMatrix, mtxPerspective);

}

//======================================
// ゲッター関数群
//======================================
const XMFLOAT4X4& Camera_GetViewMatrix() { return g_CameraViewMatrix; }
const XMFLOAT4X4& Camera_GetPerspectiveMatrix() { return g_PerspectiveMatrix; }
const XMFLOAT3& Camera_GetFront() { return g_CameraFront; }
const XMFLOAT3& Camera_GetRight() { return g_CameraRight; }
const XMFLOAT3& Camera_GetUp() { return g_CameraUp; }
const float& Camera_GetFov() { return g_Fov; }
const XMFLOAT3& Camera_GetPosition() { return g_CameraPosition; }

//======================================
// デバッグ描画
//======================================

/**
 * @brief カメラ情報デバッグ描画
 * @detail カメラ位置・方向ベクトル・FOVをテキストとして画面に重畳表示する
 */
void Camera_DebugDraw()
{
#ifdef _DEBUG
    if (!g_pDebugText) return;

    std::stringstream ss;
    ss << std::showpos << std::fixed << std::setprecision(4);
    ss << "Camera Front   : " << std::setw(8) << g_CameraFront.x << " " << std::setw(8) << g_CameraFront.y << " " << std::setw(8) << g_CameraFront.z << "\n";
    ss << "Camera Right   : " << std::setw(8) << g_CameraRight.x << " " << std::setw(8) << g_CameraRight.y << " " << std::setw(8) << g_CameraRight.z << "\n";
    ss << "Camera Up      : " << std::setw(8) << g_CameraUp.x << " " << std::setw(8) << g_CameraUp.y << " " << std::setw(8) << g_CameraUp.z << "\n";
    ss << "Camera Position: " << std::setw(8) << g_CameraPosition.x << " " << std::setw(8) << g_CameraPosition.y << " " << std::setw(8) << g_CameraPosition.z << "\n";
    ss << "Camera Fov     : " << std::setw(8) << g_Fov << "\n";

    g_pDebugText->SetText(ss.str().c_str(), Color::YELLOW);
    g_pDebugText->Draw();
    g_pDebugText->Clear();
#endif
}

//======================================
// 行列設定関数
//======================================

/**
 * @brief ビュー行列と投影行列を定数バッファに設定
 * @param view ビュー行列
 * @param projection 投影行列
 * @detail 行列を転置してGPUの定数バッファ(Slot 1, 2)へ転送する
 */
void Camera_SetMatrix(const XMMATRIX& view, const XMMATRIX& projection)
{
    // NULLチェック：バッファが未作成の場合は何もしない
    if (g_pVSConstantBuffer1 == nullptr || g_pVSConstantBuffer2 == nullptr)
    {
        return;
    }

    ID3D11DeviceContext* pContext = Direct3D_GetContext();
    if (pContext == nullptr)
    {
        return;
    }

    // 行列を転置（DirectXの行列はrow-majorだがGPUシェーダーは通常column-major期待のため）
    XMFLOAT4X4 viewTranspose, projectionTranspose;

    XMStoreFloat4x4(&viewTranspose, XMMatrixTranspose(view));
    XMStoreFloat4x4(&projectionTranspose, XMMatrixTranspose(projection));

    // 定数バッファ更新
    pContext->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &viewTranspose, 0, 0);
    pContext->UpdateSubresource(g_pVSConstantBuffer2, 0, nullptr, &projectionTranspose, 0, 0);

    // 頂点シェーダーに定数バッファを設定
    pContext->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);
    pContext->VSSetConstantBuffers(2, 1, &g_pVSConstantBuffer2);
}