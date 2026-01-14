#include "light_camera.h"

using namespace DirectX;

static XMFLOAT3 g_Position{ 0.0f, 20.0f, 0.0f }; // 初期位置も適当に入れておく
static XMFLOAT3 g_Front{ 0.0f, -1.0f, 0.0f };    // 初期値を「真下」にしてゼロを防ぐ
static XMFLOAT3 g_Up{ 0.0f, 0.0f, 1.0f };        // 真下を向くときのUpはY軸以外(Zなど)にする

// ビュー行列・プロジェクション行列のキャッシュ
static XMFLOAT4X4 g_ViewMatrix{};
static XMFLOAT4X4 g_ProjectionMatrix{};

// 影を生成する範囲
static float g_Width = 40.0f;
static float g_Height = 40.0f;
static float g_Near = 1.0f;
static float g_Far = 100.0f;

void LightCamera_Initialize(const DirectX::XMFLOAT3& world_directional, const DirectX::XMFLOAT3& world_Position)
{
    g_Front = world_directional;
    g_Position = world_Position;

    if (g_Front.x == 0 && g_Front.y == 0 && g_Front.z == 0) {
        g_Front = { 0.0f, -1.0f, 0.0f };
    }

    // 初期計算
    LightCamera_Update();
}

void LightCamera_Finalize()
{
}

void LightCamera_SetPosition(const DirectX::XMFLOAT3& position)
{
    g_Position = position;
    LightCamera_Update();
}

void LightCamera_SetFront(const DirectX::XMFLOAT3& front)
{
    g_Front = front;

    if (g_Front.x == 0 && g_Front.y == 0 && g_Front.z == 0) {
        g_Front = { 0.0f, -1.0f, 0.0f };
    }

    LightCamera_Update();
}

void LightCamera_SetRange(float w, float h, float n, float f)
{
    g_Width = w;
    g_Height = h;
    g_Near = n;
    g_Far = f;
    LightCamera_Update();
}

void LightCamera_Update()
{
    XMVECTOR pos = XMLoadFloat3(&g_Position);
    XMVECTOR dir = XMLoadFloat3(&g_Front);
    XMVECTOR up = XMLoadFloat3(&g_Up);

    // LookToLH は dir がゼロだとクラッシュ。
    // dirの長さがほぼ0なら計算しない、または正規化する処理を入れる
    if (XMVector3Equal(dir, XMVectorZero())) {
        dir = XMVectorSet(0, -1, 0, 0); // 強制リカバリ
    }
    dir = XMVector3Normalize(dir);

    // UpベクトルとDirが平行だと計算がおかしくなるので、Upを調整
    XMVECTOR cross = XMVector3Cross(dir, up);
    if (XMVector3Equal(cross, XMVectorZero())) {
        // 平行な場合、UpをX軸などにずらす
        up = XMVectorSet(1, 0, 0, 0);
    }

    XMMATRIX view = XMMatrixLookToLH(pos, dir, up);
    XMStoreFloat4x4(&g_ViewMatrix, view);

    XMMATRIX proj = XMMatrixOrthographicLH(g_Width, g_Height, g_Near, g_Far);
    XMStoreFloat4x4(&g_ProjectionMatrix, proj);
}

const DirectX::XMFLOAT4X4& LightCamera_GetViewMatrix()
{
    return g_ViewMatrix;
}

const DirectX::XMFLOAT4X4& LightCamera_GetProjectionMatrix()
{
    return g_ProjectionMatrix;
}