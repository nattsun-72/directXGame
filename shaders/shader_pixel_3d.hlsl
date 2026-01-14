/**
 * @file shader_pixel_3d.hlsl
 * @brief 3D描画用ピクセルシェーダー（ShadowMap対応・PCFソフトシャドウ・物理整合Phong版）
 * @author Natsume
 * @date 2025/12/10
 * @update 2025/12/19 - ループアンロール警告対応
 */

//=============================================================================
// 定数バッファ設定
//=============================================================================
cbuffer PS_DIFFUSE_BUFFER : register(b0)
{
    float4 diffuse_color; // マテリアルの拡散反射係数
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
    float3 eye_posW; // 視点位置
    float specular_power; // ハイライトの鋭さ
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
Texture2D tex : register(t0); // ベーステクスチャ
Texture2D g_ShadowMap : register(t1); // シャドウマップ (Depth)
SamplerState samp : register(s0); // サンプラ

//=============================================================================
// 入力構造体 (VS_OUTPUTと一致させる)
//=============================================================================
struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float4 posW : POSITION0;
    float4 normalW : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float4 wPosLight : TEXCOORD1; // ライトから見た座標
};

//=============================================================================
// ソフトシャドウ計算関数 (PCF 3x3)
//=============================================================================
float CalcShadowFactor(float4 wPosLight)
{
    // 透視除算
    float3 projCoords = wPosLight.xyz / wPosLight.w;

    // UV空間変換 (Y反転)
    float2 shadowUV;
    shadowUV.x = 0.5f * projCoords.x + 0.5f;
    shadowUV.y = -0.5f * projCoords.y + 0.5f;

    // 範囲外チェック (ライトの視界外なら影なし)
    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f ||
        shadowUV.y < 0.0f || shadowUV.y > 1.0f)
    {
        return 1.0f; // 影なし(明るい)
    }

    float currentDepth = projCoords.z;
    float bias = 0.002f;
    float shadowFactor = 0.0f;
    
    // PCF: 3x3 近傍サンプリング
    // シャドウマップ解像度(2048)の逆数オフセット
    float2 texelSize = 1.0f / 2048.0f;

    // [unroll] 属性でループを明示的にアンロール
    // これによりコンパイラ警告 X3570 を解消
    [unroll]
    for (int y = -1; y <= 1; y++)
    {
        [unroll]
        for (int x = -1; x <= 1; x++)
        {
            // 周囲の深度を取得
            float pcfDepth = g_ShadowMap.Sample(samp, shadowUV + float2(x, y) * texelSize).r;
            
            // 判定: 奥にあれば影(0.5)、手前なら光(1.0)
            shadowFactor += (currentDepth - bias > pcfDepth) ? 0.5f : 1.0f;
        }
    }
    
    // 9点の平均を返す
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
    // 影係数の取得 (ソフトシャドウ)
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
    // 影係数を適用
    float3 diffuse = baseColor * directonal_color.rgb * diffFactor * shadowFactor;

    //--------------------------------------
    // 鏡面反射 (Phong)
    //--------------------------------------
    float3 R = reflect(-L, N);
    float specFactor = pow(max(dot(R, toEye), 0.0f), specular_power);
    // 影係数を適用
    float3 specular = specular_color.rgb * specFactor * directonal_color.rgb * 0.5f * shadowFactor;

    //--------------------------------------
    // 環境光 (Ambient) - 影の影響を受けない
    //--------------------------------------
    float3 ambient = baseColor * ambient_color.rgb;

    //--------------------------------------
    // 合成 (Directional Light)
    //--------------------------------------
    float3 color = ambient + diffuse + specular;

    //--------------------------------------
    // 点光源処理 (Point Lights) - 影計算なし
    //--------------------------------------
    [unroll(4)] // 最大4つの点光源をアンロール
    for (int i = 0; i < pointLightCount; i++)
    {
        float3 lightToPixel = ps_in.posW.xyz - pointLight[i].posW;
        float D = length(lightToPixel);
        
        // 減衰計算
        float A = pow(max(1.0f - 1.0f / pointLight[i].range * D, 0.0f), 2.0f);

        // 拡散反射
        float3 L_point = -normalize(lightToPixel);
        float dl = max(0.0f, dot(L_point, N));
        color += baseColor * pointLight[i].color.rgb * A * dl * 0.5f;

        // 鏡面反射
        float3 r_point = reflect(normalize(lightToPixel), N);
        float t = pow(max(dot(r_point, toEye), 0.0f), specular_power);
        float3 specularPoint = pointLight[i].color.rgb * t * 0.5f;
        
        color += specularPoint;
    }
    
    return float4(saturate(color), alpha);
}