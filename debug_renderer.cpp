/****************************************
 * @file    debug_renderer.cpp
 * @brief   デバッグ描画の実装
 * @detail  LINELISTトポロジを用いた簡易プリミティブ描画
 ****************************************/

#include "debug_renderer.h"
#include "direct3d.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <string>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

namespace DebugRenderer
{
    //--------------------------------------
    // 内部構造体定義
    //--------------------------------------
    struct DebugVertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };

    struct ConstantBuffer
    {
        XMFLOAT4X4 wvp;
    };

    //--------------------------------------
    // 内部リソース
    //--------------------------------------
    static ID3D11Buffer* g_pVertexBuffer = nullptr;
    static ID3D11Buffer* g_pConstantBuffer = nullptr;
    static ID3D11VertexShader* g_pVertexShader = nullptr;
    static ID3D11PixelShader* g_pPixelShader = nullptr;
    static ID3D11InputLayout* g_pInputLayout = nullptr;

    static std::vector<DebugVertex> g_LineList;

    //======================================
    // シェーダーコード
    //======================================
    static const char* g_ShaderCode = R"(
        cbuffer ConstantBuffer : register(b0) {
            matrix WVP;
        };
        struct VS_INPUT {
            float3 pos : POSITION;
            float4 col : COLOR;
        };
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float4 col : COLOR;
        };
        PS_INPUT VS(VS_INPUT input) {
            PS_INPUT output;
            output.pos = mul(float4(input.pos, 1.0f), WVP);
            output.col = input.col;
            return output;
        }
        float4 PS(PS_INPUT input) : SV_Target {
            return input.col;
        }
    )";

    //======================================
    // 基本動作関数群
    //======================================

    void Initialize()
    {
        ID3D11Device* dev = Direct3D_GetDevice();
        if (!dev) return;

        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;

        // 頂点シェーダーのコンパイルと作成
        HRESULT hr = D3DCompile(g_ShaderCode, strlen(g_ShaderCode), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) errorBlob->Release();
            return;
        }
        dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_pVertexShader);

        // ピクセルシェーダーのコンパイルと作成
        hr = D3DCompile(g_ShaderCode, strlen(g_ShaderCode), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, nullptr);
        if (SUCCEEDED(hr)) {
            dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_pPixelShader);
            psBlob->Release();
        }

        // 入力レイアウトの作成
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        dev->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_pInputLayout);
        vsBlob->Release();

        // 頂点バッファの作成（動的更新用）
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = sizeof(DebugVertex) * 20000; // 容量を少し増やして2万頂点
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        dev->CreateBuffer(&bd, nullptr, &g_pVertexBuffer);

        // 定数バッファの作成
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(ConstantBuffer);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        dev->CreateBuffer(&bd, nullptr, &g_pConstantBuffer);
    }

    void Finalize()
    {
        if (g_pInputLayout) { g_pInputLayout->Release();    g_pInputLayout = nullptr; }
        if (g_pVertexShader) { g_pVertexShader->Release();   g_pVertexShader = nullptr; }
        if (g_pPixelShader) { g_pPixelShader->Release();    g_pPixelShader = nullptr; }
        if (g_pVertexBuffer) { g_pVertexBuffer->Release();   g_pVertexBuffer = nullptr; }
        if (g_pConstantBuffer) { g_pConstantBuffer->Release(); g_pConstantBuffer = nullptr; }
    }

    //======================================
    // 描画登録関数群
    //======================================

    void DrawLine(const XMFLOAT3& p1, const XMFLOAT3& p2, const XMFLOAT4& color)
    {
        g_LineList.push_back({ p1, color });
        g_LineList.push_back({ p2, color });
    }

    void DrawAABB(const AABB& aabb, const XMFLOAT4& color)
    {
        XMFLOAT3 p[8] = {
            { aabb.min.x, aabb.min.y, aabb.min.z }, { aabb.max.x, aabb.min.y, aabb.min.z },
            { aabb.min.x, aabb.max.y, aabb.min.z }, { aabb.max.x, aabb.max.y, aabb.min.z },
            { aabb.min.x, aabb.min.y, aabb.max.z }, { aabb.max.x, aabb.min.y, aabb.max.z },
            { aabb.min.x, aabb.max.y, aabb.max.z }, { aabb.max.x, aabb.max.y, aabb.max.z }
        };

        // 12本の線で構成
        DrawLine(p[0], p[1], color); DrawLine(p[1], p[5], color);
        DrawLine(p[5], p[4], color); DrawLine(p[4], p[0], color);
        DrawLine(p[2], p[3], color); DrawLine(p[3], p[7], color);
        DrawLine(p[7], p[6], color); DrawLine(p[6], p[2], color);
        DrawLine(p[0], p[2], color); DrawLine(p[1], p[3], color);
        DrawLine(p[5], p[7], color); DrawLine(p[4], p[6], color);
    }

    void DrawOBB(const OBB& obb, const XMFLOAT4& color)
    {
        XMVECTOR c = XMLoadFloat3(&obb.center);
        XMVECTOR e = XMLoadFloat3(&obb.extents);
        XMMATRIX matRot = XMMatrixRotationQuaternion(XMLoadFloat4(&obb.orientation));

        // 基底ベクトルに範囲を乗算
        XMVECTOR u0 = matRot.r[0] * XMVectorGetX(e);
        XMVECTOR u1 = matRot.r[1] * XMVectorGetY(e);
        XMVECTOR u2 = matRot.r[2] * XMVectorGetZ(e);

        XMVECTOR corners[8];
        corners[0] = c - u0 - u1 - u2; corners[1] = c + u0 - u1 - u2;
        corners[2] = c - u0 + u1 - u2; corners[3] = c + u0 + u1 - u2;
        corners[4] = c - u0 - u1 + u2; corners[5] = c + u0 - u1 + u2;
        corners[6] = c - u0 + u1 + u2; corners[7] = c + u0 + u1 + u2;

        XMFLOAT3 p[8];
        for (int i = 0; i < 8; ++i) XMStoreFloat3(&p[i], corners[i]);

        DrawLine(p[0], p[1], color); DrawLine(p[1], p[5], color);
        DrawLine(p[5], p[4], color); DrawLine(p[4], p[0], color);
        DrawLine(p[2], p[3], color); DrawLine(p[3], p[7], color);
        DrawLine(p[7], p[6], color); DrawLine(p[6], p[2], color);
        DrawLine(p[0], p[2], color); DrawLine(p[1], p[3], color);
        DrawLine(p[5], p[7], color); DrawLine(p[4], p[6], color);
    }

    void DrawSphere(const Sphere& sphere, const XMFLOAT4& color)
    {
        const int SEGMENTS = 16;
        XMVECTOR c = XMLoadFloat3(&sphere.center);
        float r = sphere.radius;

        // 各軸周りに円を描画
        for (int axis = 0; axis < 3; ++axis)
        {
            // 警告 C4701 回避のため、初期頂点をループ外で計算
            XMVECTOR prevPos;
            {
                XMVECTOR startOffset;
                if (axis == 0)      startOffset = XMVectorSet(0.0f, 0.0f, r, 0.0f);
                else if (axis == 1) startOffset = XMVectorSet(0.0f, 0.0f, r, 0.0f);
                else                startOffset = XMVectorSet(0.0f, r, 0.0f, 0.0f);
                prevPos = c + startOffset;
            }

            for (int i = 1; i <= SEGMENTS; ++i)
            {
                float angle = (static_cast<float>(i) / SEGMENTS) * XM_2PI;
                float s = sinf(angle) * r;
                float cs = cosf(angle) * r;

                XMVECTOR offset;
                if (axis == 0)      offset = XMVectorSet(0.0f, s, cs, 0.0f);    // YZ
                else if (axis == 1) offset = XMVectorSet(s, 0.0f, cs, 0.0f);    // XZ
                else                offset = XMVectorSet(s, cs, 0.0f, 0.0f);    // XY

                XMVECTOR currentPos = c + offset;

                XMFLOAT3 p1, p2;
                XMStoreFloat3(&p1, prevPos);
                XMStoreFloat3(&p2, currentPos);
                DrawLine(p1, p2, color);

                prevPos = currentPos;
            }
        }
    }

    void DrawGrid(float size, int divisions, const XMFLOAT4& color)
    {
        float step = size / divisions;
        float half = size / 2.0f;

        for (int i = 0; i <= divisions; ++i)
        {
            float pos = -half + i * step;
            DrawLine({ pos, 0.0f, -half }, { pos, 0.0f, half }, color);
            DrawLine({ -half, 0.0f, pos }, { half, 0.0f, pos }, color);
        }
    }

    //======================================
    // 描画実行処理
    //======================================

    void Render(const XMFLOAT4X4& view, const XMFLOAT4X4& proj)
    {
        if (g_LineList.empty()) return;

        ID3D11DeviceContext* ctx = Direct3D_GetContext();
        if (!ctx) return;

        // 頂点バッファの更新
        D3D11_MAPPED_SUBRESOURCE ms;
        size_t drawCount = g_LineList.size();

        // バッファサイズ超過を防ぐガード（必要に応じてバッファ再生成を検討）
        if (drawCount > 20000) drawCount = 20000;

        if (SUCCEEDED(ctx->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
        {
            memcpy(ms.pData, g_LineList.data(), drawCount * sizeof(DebugVertex));
            ctx->Unmap(g_pVertexBuffer, 0);
        }

        // 定数バッファの更新
        XMMATRIX mView = XMLoadFloat4x4(&view);
        XMMATRIX mProj = XMLoadFloat4x4(&proj);
        XMMATRIX mWVP = mView * mProj;

        ConstantBuffer cb;
        XMStoreFloat4x4(&cb.wvp, XMMatrixTranspose(mWVP));
        ctx->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb, 0, 0);

        // パイプライン設定
        ctx->IASetInputLayout(g_pInputLayout);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

        UINT stride = sizeof(DebugVertex);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

        ctx->VSSetShader(g_pVertexShader, nullptr, 0);
        ctx->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
        ctx->PSSetShader(g_pPixelShader, nullptr, 0);

        ctx->Draw(static_cast<UINT>(drawCount), 0);

        // 次フレームに向けてリストをクリア
        g_LineList.clear();
    }
}