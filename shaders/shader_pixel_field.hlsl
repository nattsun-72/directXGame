/**
 * @file shader_pixel_field.hlsl
 * @brief 3Dフィールド描画用ピクセルシェーダー
 * @author Natsume Shidara
 * @date 2025/06/10
 * @UpDate 2025/10/29
 */
cbuffer PS_DIFFUSE_BUFFER : register(b0)
{
    float4 diffuse_color; // マテリアルの拡散反射係数（RGBA）
};

cbuffer PS_AMBIENT_BUFFER : register(b1)
{
    float4 ambient_color; // 環境光
};

cbuffer PS_DIRECTIONAL_BUFFER : register(b2)
{
    float4 directonal_world_vector; // 光の方向（ワールド空間）
    float4 directonal_color; // 平行光の色
};

cbuffer PS_SPECULAR_BUFFER : register(b3)
{
    float4 specular_color; // 鏡面反射色
    float3 eye_posW; // 視点位置（ワールド座標）
    float specular_power; // 鏡面ハイライトの鋭さ
};

#define PI 3.1415926535897932384626433f



struct PS_INPUT
{
    float4 posH : SV_POSITION; // クリップ空間位置
    float4 posW : POSITION0; // ワールド空間位置
    float4 normalW : NORMAL0; // ワールド空間法線（float4）
    float4 blend : COLOR0; // 頂点カラー
    float2 uv : TEXCOORD0; // テクスチャ座標
};

Texture2D tex0 : register(t0); // テクスチャ
Texture2D tex1 : register(t1); // テクスチャ

SamplerState samp : register(s0);


float4 main(PS_INPUT ps_in) : SV_TARGET
{
    //--------------------------------------
    // テクスチャ合成（rチャンネルをブレンド係数として使用）
    //--------------------------------------
    float2 uv = ps_in.uv;
    float blendRate = saturate(ps_in.blend.r);

    float4 tex0Color = tex0.Sample(samp, ps_in.uv);
    float4 tex1Color = tex1.Sample(samp, ps_in.uv);

    //--------------------------------------
    // 頂点カラーのRとGをブレンド係数として使用
    // （R=tex1寄り、G=tex0寄り）
    //--------------------------------------
    float4 tex_Color = tex0Color * ps_in.blend.g + tex1Color * ps_in.blend.r;

    // αも同様にブレンド
    float alpha = tex0Color.a * ps_in.blend.g + tex1Color.a * ps_in.blend.r;


    //--------------------------------------
    // 光計算用ベクトル
    //--------------------------------------
    float3 N = normalize(ps_in.normalW.xyz);
    float3 L = normalize(-directonal_world_vector.xyz);
    float3 toEye = normalize(eye_posW - ps_in.posW.xyz);
    float3 R = reflect(-L, N);

    //--------------------------------------
    // ライティング計算
    //--------------------------------------
    float diffFactor = max(dot(N, L), 0.0f);
    float3 diffuse = tex_Color.rgb * directonal_color.rgb * diffFactor;
    float3 ambient = tex_Color.rgb * ambient_color.rgb;

    float specFactor = pow(max(dot(R, toEye), 0.0f), specular_power);
    float3 specular = specular_color.rgb * specFactor * directonal_color.rgb;

    //--------------------------------------
    // 出力合成
    //--------------------------------------
    float3 color = ambient + diffuse + specular;
    
    //引数指定で書く書かないを定められる。
    //clip(sin(ps_in.uv.x * (ps_in.uv.y * 100000000)));
    
    return float4(saturate(color), alpha);
}
