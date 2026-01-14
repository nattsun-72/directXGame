/****************************************
 * @file shader_vertex_3d_unlit.hlsl
 * @brief 3D用頂点アンリットシェーダー
 * @author Natsume Shidara
 * @date 2025/11/21
 * @update 2025/11/21
 ****************************************/

//--------------------------------------
// 定数バッファ定義
//--------------------------------------
// ワールド行列 (Individual)
cbuffer WorldBuffer : register(b0)
{
    matrix g_World;
}

// ビュー行列 (Shared/Global)
cbuffer ViewBuffer : register(b1)
{
    matrix g_View;
}

// プロジェクション行列 (Shared/Global)
cbuffer ProjectionBuffer : register(b2)
{
    matrix g_Proj;
}

//--------------------------------------
// 構造体定義
//--------------------------------------
// 頂点シェーダー入力構造体
// ※C++側 D3D11_INPUT_ELEMENT_DESC に完全準拠
struct VS_INPUT
{
    float3 posL : POSITION0; // ローカル座標
    float3 normal : NORMAL0; // 法線 (Unlitでは計算未使用だがレイアウト整合のため定義)
    float4 color : COLOR0; // 頂点カラー
    float2 uv : TEXCOORD0; // テクスチャ座標
};

// 頂点シェーダー出力構造体
struct VS_OUTPUT
{
    float4 posH : SV_POSITION; // 変換済み座標 (Clip Space)
    float4 color : COLOR0; // 頂点カラー
    float2 uv : TEXCOORD0; // テクスチャ座標
};

//======================================
// メイン関数
//======================================
/**
 * @brief 頂点シェーダーエントリーポイント
 * @param vs_in 入力頂点データ
 * @return ピクセルシェーダーへの出力データ
 * @detail World -> View -> Projection の順で座標変換を行います。
 */
VS_OUTPUT main(VS_INPUT vs_in)
{
    VS_OUTPUT vs_out;

    // 入力座標(float3)を同次座標(float4)に拡張 w=1.0
    float4 pos = float4(vs_in.posL, 1.0f);

    // 座標変換: Local -> World
    pos = mul(pos, g_World);

    // 座標変換: World -> View
    pos = mul(pos, g_View);

    // 座標変換: View -> Projection
    pos = mul(pos, g_Proj);

    // 結果を出力
    vs_out.posH = pos;
    vs_out.color = vs_in.color; // 頂点カラーをパススルー
    vs_out.uv = vs_in.uv; // UVをパススルー

    return vs_out;
}