/**
 * @file shader_postprocess_radial_ps.hlsl
 * @brief ラジアルブラー（放射状ブラー）ピクセルシェーダー
 * @detail ダッシュ時の疾走感演出用。画面中央から外側に向かってブラー
 * @author 
 * @date 2026/01/10
 */

//=============================================================================
// 定数バッファ
//=============================================================================
cbuffer PostProcessCB : register(b0)
{
    float2 Center;      // ブラー中心（UV座標、通常は0.5, 0.5）
    float Intensity;    // ブラー強度（0.0〜1.0）
    float SampleCount;  // サンプル数（通常16）
    float2 Direction;   // 方向性ブラー用（ラジアルでは未使用）
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
    
    // 中心からの方向ベクトル
    float2 dir = texCoord - Center;
    float dist = length(dir);
    
    // 中心からの距離に応じてブラー強度を変える
    // 中心は弱く、端は強く（疾走感を出す）
    float blurAmount = Intensity * dist * 0.5f;
    
    // サンプリング
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
    
    int samples = (int)SampleCount;
    
    [unroll(16)] // 最大16サンプル
    for (int i = 0; i < samples; i++)
    {
        float t = (float)i / (float)(samples - 1);
        
        // 中心に向かってスケール
        float scale = 1.0f - blurAmount * t;
        float2 sampleCoord = Center + dir * scale;
        
        // 画面外をクランプ
        sampleCoord = clamp(sampleCoord, 0.0f, 1.0f);
        
        // 中心に近いほど重み大
        float weight = 1.0f - t * 0.5f;
        color += SceneTexture.Sample(LinearSampler, sampleCoord) * weight;
        totalWeight += weight;
    }
    
    return color / totalWeight;
}
