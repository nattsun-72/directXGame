/**
 * @file shader_pixel_billboard.hlsl
 * @brief ビルボード用ピクセルシェーダー
 * @author Natsume Shidara
 * @date 2025/11/14
 */

// 定数バッファ (register b0: Color)
cbuffer ColorBuffer : register(b0)
{
    float4 materialColor;
};

// ピクセルシェーダー入力構造体
struct PS_INPUT
{
    float4 posH : SV_POSITION; // 変換済み座標
    float4 color : COLOR0; // 頂点カラー
    float2 uv : TEXCOORD0; // テクスチャ座標
};

// テクスチャとサンプラー
Texture2D tex : register(t0); // テクスチャ
SamplerState samp : register(s0); // サンプラー

// ピクセルシェーダー
float4 main(PS_INPUT ps_in) : SV_TARGET
{
    // テクスチャサンプリング
    float4 texColor = tex.Sample(samp, ps_in.uv);
    
    
    //アルファテスト
    if (texColor.a < 0.01f)
    {
        //引数指定で書く書かないを定められる。
        clip(-1);
    }
    
    // 頂点カラー、マテリアルカラー、テクスチャカラーを乗算
        return texColor * ps_in.color * materialColor;
}