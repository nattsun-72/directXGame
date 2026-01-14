/**
 * @file shader_vertex_3d.hlsl
 * @brief 3D描画用頂点シェーダー（ShadowMap対応版）
 * @author Natsume Shidara
 * @date 2025/12/10
 */

//=============================================================================
// 定数バッファ設定
//=============================================================================
// スロット0: ワールド行列
cbuffer VS_WORLD : register(b0)
{
    float4x4 world;
};

// スロット1: ビュー行列
cbuffer VS_VIEW : register(b1)
{
    float4x4 view;
};

// スロット2: プロジェクション行列
cbuffer VS_PROJ : register(b2)
{
    float4x4 proj;
};


cbuffer VS_SHADOW : register(b3)
{
    float4x4 LightViewProjection;
};

//=============================================================================
// 入出力構造体
//=============================================================================
struct VS_INPUT
{
    float4 posL : POSITION0; // ローカル座標 
    float4 normalL : NORMAL0; // 法線
    float4 color : COLOR0; // Color
    float2 uv : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 posH : SV_POSITION; // クリップ空間座標
    float4 posW : POSITION0; // ワールド座標
    float4 normalW : NORMAL0; // ワールド法線
    float4 color : COLOR0; // Color
    float2 uv : TEXCOORD0;
    float4 wPosLight : TEXCOORD1; 
    
};

//=============================================================================
// 頂点シェーダメイン
//=============================================================================
VS_OUTPUT main(VS_INPUT vs_in)
{
    VS_OUTPUT vs_out;

    // 1. 座標変換 (Local -> World -> View -> Proj)
    float4 mtxW = mul(vs_in.posL, world);
    float4 mtxWV = mul(mtxW, view);
    float4 mtxH = mul(mtxWV, proj);
    
    vs_out.posH = mtxH;
    vs_out.posW = mtxW;

    // 2. 法線変換
    // ワールド行列の回転成分のみを適用するため、第4成分を0にして計算
    vs_out.normalW = normalize(mul(float4(vs_in.normalL.xyz, 0.0f), world));
    
    // 3. 色・UVパススルー
    vs_out.color = vs_in.color;
    vs_out.uv = vs_in.uv;

    vs_out.wPosLight = mul(mtxW, LightViewProjection);

    return vs_out;
}