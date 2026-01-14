/**
 * @file shader_vertex_billboard.hlsl
 * @brief ビルボード用頂点シェーダー
 * @author Natsume Shidara
 * @date 2025/11/14
 */

cbuffer VS_CONSTANT_BUFFER : register(b0)
{
    float4x4 world;
};
cbuffer VS_CONSTANT_BUFFER : register(b1)
{
    float4x4 view;
};
cbuffer VS_CONSTANT_BUFFER : register(b2)
{
    float4x4 proj;
};


cbuffer UVOffsetBuffer : register(b3)
{
    float2 scale;
    float2 translation;
};

// 頂点シェーダー入力構造体
struct VS_INPUT
{
    float3 posL : POSITION0; // ローカル座標 (float3に変更)
    float4 color : COLOR0; // 頂点カラー
    float2 uv : TEXCOORD0; // テクスチャ座標
};

// 頂点シェーダー出力構造体
struct VS_OUTPUT
{
    float4 posH : SV_POSITION; // 変換済み座標
    float4 color : COLOR0; // カラー
    float2 uv : TEXCOORD0; // テクスチャ座標
};

// 頂点シェーダー
VS_OUTPUT main(VS_INPUT vs_in)
{
    VS_OUTPUT vs_out;
    
    // 座標変換 (float3 -> float4に拡張)
    float4 posL = float4(vs_in.posL, 1.0f);
    float4 posW = mul(posL, world);
    float4 posV = mul(posW, view);
    float4 posH = mul(posV, proj);
    
    vs_out.posH = posH;
    vs_out.color = vs_in.color;
    vs_out.uv = vs_in.uv * scale + translation;
    
    return vs_out;
}