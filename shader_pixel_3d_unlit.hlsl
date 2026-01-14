/****************************************
 * @file shader_pixel_3d_unlit.hlsl
 * @brief 3D用ピクセルアンリットシェーダー
 * @author Natsume Shidara
 * @date 2025/11/21
 * @update 2025/11/21
 ****************************************/

//--------------------------------------
// 定数バッファ定義
//--------------------------------------
// マテリアルカラー
cbuffer ColorBuffer : register(b0)
{
    float4 MaterialColor;
}

//--------------------------------------
// 構造体定義
//--------------------------------------
// ピクセルシェーダー入力構造体
// ※VS_OUTPUTと一致させる
struct PS_INPUT
{
    float4 posH : SV_POSITION; // スクリーン座標
    float4 color : COLOR0; // 頂点カラー
    float2 uv : TEXCOORD0; // テクスチャ座標
};

//--------------------------------------
// テクスチャ・サンプラー定義
//--------------------------------------
Texture2D Tex : register(t0); // ベーステクスチャ
SamplerState Samp : register(s0); // サンプラー

//======================================
// メイン関数
//======================================
/**
 * @brief ピクセルシェーダーエントリーポイント
 * @param ps_in ラスタライズ済みデータ
 * @return 最終出力カラー
 * @detail ライティング計算を行わず、テクスチャとカラーの乗算結果を返します。
 */
float4 main(PS_INPUT ps_in) : SV_TARGET
{
    // テクスチャカラー抽出
    float4 texColor = Tex.Sample(Samp, ps_in.uv);

    // 最終カラー合成
    // Texture * Material * VertexColor
    return texColor;
}