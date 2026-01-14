/**
 * @file shader_postprocess_vs.hlsl
 * @brief ポストプロセス用フルスクリーン頂点シェーダー
 * @detail 頂点バッファなしで3頂点のフルスクリーン三角形を生成
 * @author 
 * @date 2026/01/10
 */

//=============================================================================
// 出力構造体
//=============================================================================
struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

//=============================================================================
// 頂点シェーダーメイン
// @param vertexID 頂点ID（0, 1, 2）
// @return フルスクリーン三角形の頂点
//=============================================================================
VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    
    // 3頂点でフルスクリーン三角形を描画
    // vertexID: 0 -> (0, 0), 1 -> (2, 0), 2 -> (0, 2)
    output.TexCoord = float2((vertexID << 1) & 2, vertexID & 2);
    
    // UV座標をクリップ座標に変換
    // (0,0) -> (-1, 1), (1,0) -> (1, 1), (0,1) -> (-1, -1), (1,1) -> (1, -1)
    output.Position = float4(output.TexCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    
    return output;
}
