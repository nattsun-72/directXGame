/**
 * @file shader_postprocess_directional_ps.hlsl
 * @brief 方向性ブラー（モーションブラー）ピクセルシェーダー
 * @detail ステップ回避時の方向感演出用。指定方向にブラー
 * @author 
 * @date 2026/01/10
 */

//=============================================================================
// 定数バッファ
//=============================================================================
cbuffer PostProcessCB : register(b0)
{
    float2 Center;      // 未使用（ラジアル用）
    float Intensity;    // ブラー強度（0.0〜1.0）
    float SampleCount;  // サンプル数（通常12）
    float2 Direction;   // ブラー方向（正規化ベクトル）
    float2 Padding;     // パディング
};

//=============================================================================
// テクスチャ・サンプラ
//=============================================================================
Texture2D SceneTexture : register(t0);
SamplerState LinearSampler : register(s0);

//=============================================================================
// 入力構造体
//=============================================================================
struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

//=============================================================================
// ピクセルシェーダーメイン
//=============================================================================
float4 main(PS_INPUT input) : SV_TARGET
{
    float2 texCoord = input.TexCoord;
    
    // ブラー方向のオフセット
    float2 blurDir = Direction * Intensity * 0.02f;
    
    // サンプリング
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
    
    int samples = (int)SampleCount;
    
    [unroll(12)] // 最大12サンプル
    for (int i = 0; i < samples; i++)
    {
        // -0.5 〜 +0.5 の範囲
        float t = ((float)i / (float)(samples - 1)) - 0.5f;
        
        // ブラー方向にオフセット
        float2 offset = blurDir * t * 2.0f;
        float2 sampleCoord = texCoord + offset;
        
        // 画面外をクランプ
        sampleCoord = clamp(sampleCoord, 0.0f, 1.0f);
        
        // 中心に近いほど重み大（三角分布）
        float weight = 1.0f - abs(t);
        color += SceneTexture.Sample(LinearSampler, sampleCoord) * weight;
        totalWeight += weight;
    }
    
    return color / totalWeight;
}
