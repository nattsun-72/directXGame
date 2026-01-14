/**
 * @file shader_veretx_field.hlsl
 * @brief メッシュフィールド描画用頂点シェーダー
 * @author Natsume Shidara
 * @date 2025/10/20
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


struct VS_INPUT
{
    float4 posL : POSITION0; // ローカル座標 
    float4 normalL : NORMAL0; // 法線
    float4 blend : COLOR0; // Color
    float2 uv : TEXCOORD0;
};
struct VS_OUTPUT
{
    float4 posH : SV_POSITION; // 変換済み座標 
    float4 posW : POSITION0;
    float4 normalW : NORMAL0;
    float4 blend : COLOR0; // Color
    float2 uv : TEXCOORD0;
};

// 頂点シェーダ
VS_OUTPUT main(VS_INPUT vs_in)
{
    VS_OUTPUT vs_out;

    // 座標変換
    //float4x4 mtxWV = mul(world, view);
    float4x4 mtxWV = mul(view, world);
    float4x4 mtxWVP = mul(mtxWV, proj);
    vs_out.posH = mul(vs_in.posL, mtxWVP);
    
    vs_out.normalW = normalize(mul(float4(vs_in.normalL.xyz, 0.0f), world));

    vs_out.posW = mul(vs_in.posL, world);
    
    // 色計算
    vs_out.blend = vs_in.blend; //地面のテクスチャのブレンド値はそのままパススルー
    vs_out.uv = vs_in.uv;
    
    return vs_out;
}
