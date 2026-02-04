/**
 * @file shader_pixel_3d.hlsl
 * @brief 3D描画用ピクセルシェーダー（ShadowMap対応・PCFソフトシャドウ・物理整合Phong版）
 * @author Natsume
 * @date 2025/12/10
 * @update 2025/12/19 - ループアンロール警告対応
 * @update 2026/02/03 - ライティング調整（明るさ改善）
 */

//=============================================================================
// 定数バッファ設定
//=============================================================================
cbuffer PS_DIFFUSE_BUFFER : register(b0)
{
    float4 diffuse_color;
};

cbuffer PS_AMBIENT_BUFFER : register(b1)
{
    float4 ambient_color;
};

cbuffer PS_DIRECTIONAL_BUFFER : register(b2)
{
    float4 directonal_world_vector;
    float4 directonal_color;
};

cbuffer PS_SPECULAR_BUFFER : register(b3)
{
    float4 specular_color;
    float3 eye_posW;
    float specular_power;
};

struct PointLight
{
    float3 posW;
    float range;
    float4 color;
};

cbuffer PS_POINTLIGHT_BUFFER : register(b4)
{
    PointLight pointLight[4];
    int pointLightCount;
    float3 pointLight_dummy;
};

//=============================================================================
// テクスチャ・サンプラ
//=============================================================================
Texture2D tex : register(t0);
Texture2D g_ShadowMap : register(t1);
SamplerState samp : register(s0);

//=============================================================================
// 入力構造体
//=============================================================================
struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float4 posW : POSITION0;
    float4 normalW : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float4 wPosLight : TEXCOORD1;
};

//=============================================================================
// ライティング調整パラメータ
//=============================================================================
#define SHADOW_MIN          0.6f
#define SHADOW_BIAS         0.002f
#define DIFFUSE_INTENSITY   1.1f
#define SPECULAR_INTENSITY  0.6f
#define POINT_DIFFUSE       0.6f
#define POINT_SPECULAR      0.5f

//=============================================================================
// ソフトシャドウ計算関数 (PCF 3x3)
//=============================================================================
float CalcShadowFactor(float4 wPosLight)
{
    float3 projCoords = wPosLight.xyz / wPosLight.w;

    float2 shadowUV;
    shadowUV.x = 0.5f * projCoords.x + 0.5f;
    shadowUV.y = -0.5f * projCoords.y + 0.5f;

    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f ||
        shadowUV.y < 0.0f || shadowUV.y > 1.0f)
    {
        return 1.0f;
    }

    float currentDepth = projCoords.z;
    float shadowFactor = 0.0f;
    
    float2 texelSize = 1.0f / 2048.0f;

    [unroll]
    for (int y = -1; y <= 1; y++)
    {
        [unroll]
        for (int x = -1; x <= 1; x++)
        {
            float pcfDepth = g_ShadowMap.Sample(samp, shadowUV + float2(x, y) * texelSize).r;
            shadowFactor += (currentDepth - SHADOW_BIAS > pcfDepth) ? SHADOW_MIN : 1.0f;
        }
    }
    
    return shadowFactor / 9.0f;
}

//=============================================================================
// メインシェーダー
//=============================================================================
float4 main(PS_INPUT ps_in) : SV_TARGET
{
    //--------------------------------------
    // 基本色生成
    //--------------------------------------
    float4 texColor = tex.Sample(samp, ps_in.uv);
    float3 baseColor = texColor.rgb * ps_in.color.rgb * diffuse_color.rgb;
    float alpha = texColor.a * ps_in.color.a * diffuse_color.a;

    //--------------------------------------
    // 影係数の取得
    //--------------------------------------
    float shadowFactor = CalcShadowFactor(ps_in.wPosLight);

    //--------------------------------------
    // ライティング計算準備
    //--------------------------------------
    float3 N = normalize(ps_in.normalW.xyz);
    float3 L = normalize(-directonal_world_vector.xyz);
    float3 toEye = normalize(eye_posW - ps_in.posW.xyz);

    //--------------------------------------
    // 拡散反射 (Lambert)
    //--------------------------------------
    float diffFactor = max(dot(N, L), 0.0f);
    float3 diffuse = baseColor * directonal_color.rgb * diffFactor * shadowFactor * DIFFUSE_INTENSITY;

    //--------------------------------------
    // 鏡面反射 (Phong)
    //--------------------------------------
    float3 R = reflect(-L, N);
    float specFactor = pow(max(dot(R, toEye), 0.0f), specular_power);
    float3 specular = specular_color.rgb * specFactor * directonal_color.rgb * SPECULAR_INTENSITY * shadowFactor;

    //--------------------------------------
    // 環境光 (Ambient)
    //--------------------------------------
    float3 ambient = baseColor * ambient_color.rgb;

    //--------------------------------------
    // 合成 (Directional Light)
    //--------------------------------------
    float3 color = ambient + diffuse + specular;

    //--------------------------------------
    // 点光源処理 (Point Lights)
    //--------------------------------------
    [unroll(4)]
    for (int i = 0; i < pointLightCount; i++)
    {
        float3 lightToPixel = ps_in.posW.xyz - pointLight[i].posW;
        float D = length(lightToPixel);
        
        float A = pow(max(1.0f - 1.0f / pointLight[i].range * D, 0.0f), 2.0f);

        float3 L_point = -normalize(lightToPixel);
        float dl = max(0.0f, dot(L_point, N));
        color += baseColor * pointLight[i].color.rgb * A * dl * POINT_DIFFUSE;

        float3 r_point = reflect(normalize(lightToPixel), N);
        float t = pow(max(dot(r_point, toEye), 0.0f), specular_power);
        color += pointLight[i].color.rgb * t * A * POINT_SPECULAR;
    }
    
    return float4(saturate(color), alpha);
}