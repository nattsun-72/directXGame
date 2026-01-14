/****************************************
 * @file light.h
 * @brief ライト関連の設定およびGPU転送用構造体定義ヘッダ
 *
 * 本ヘッダは、DirectX 11環境で使用するライト情報（環境光・平行光・鏡面光・ポイントライト）を
 * HLSLの定数バッファ構造体と整合する形で定義し、
 * GPUへのデータ転送を行う初期化／設定関数を宣言する。
 *
 * - Ambient（環境光）
 * - Directional（平行光）
 * - Specular（鏡面反射光）
 * - Point（点光源、最大4灯対応）
 *
 * @author
 *   Natsume Shidara
 * @date
 *   2025/09/11
 ****************************************/

#ifndef LIGHT_H
#define LIGHT_H

//--------------------------------------
// インクルードガード／依存ヘッダ
//--------------------------------------
#include <DirectXMath.h>
#include <d3d11.h>

using namespace DirectX;

//======================================
// 定数バッファ構造体群（HLSL対応）
//======================================

/****************************************
 * @struct AmbientCB
 * @brief 環境光（アンビエントライト）情報
 *
 * HLSL側の `cbuffer PS_AMBIENT_BUFFER` と整合。
 * シーン全体の基礎的な明るさを表す。
 ****************************************/
struct AmbientCB
{
    XMFLOAT4 ambient_color; // 環境光の色（RGBA形式）
};

/****************************************
 * @struct DirectionalLightCB
 * @brief 平行光源（ディレクショナルライト）情報
 *
 * HLSL側の `cbuffer PS_DIRECTIONAL_BUFFER` に対応。
 * ワールド空間での光の向きと色を格納。
 ****************************************/
struct DirectionalLightCB
{
    XMFLOAT4 directonal_world_vector; // ワールド空間上の光の方向ベクトル
    XMFLOAT4 directonal_color; // 光の色（RGBA形式）
};

/****************************************
 * @struct SpecularLightCB
 * @brief 鏡面反射光（スペキュラーライト）情報
 *
 * HLSL側の `cbuffer PS_SPECULAR_BUFFER` に対応。
 * 光沢の強さやカメラ位置を考慮してハイライトを計算。
 ****************************************/
struct SpecularLightCB
{
    XMFLOAT4 specular_color; // 鏡面反射光の色（RGBA形式）
    XMFLOAT3 eye_posW; // カメラのワールド座標（視点位置）
    float specular_power; // 光沢強度（大きいほど鋭い反射）
};

/****************************************
 * @struct PointLightCB
 * @brief 点光源（ポイントライト）情報
 *
 * HLSL側構造体：
 *   PointLight { float3 posW; float range; float4 color; };
 *
 * 光源の位置・影響範囲・発光色を保持。
 ****************************************/
struct PointLightCB
{
    XMFLOAT3 posW; // 点光源のワールド座標
    float range; // 有効距離（光が届く範囲）
    XMFLOAT4 color; // 発光色（RGBA形式）
};

/****************************************
 * @struct PointLightListCB
 * @brief 点光源リスト構造体（最大4灯対応）
 *
 * 複数の点光源をGPUへまとめて送信するための構造体。
 * 配列要素数とダミー変数で16バイト境界を維持。
 ****************************************/
struct PointLightListCB
{
    PointLightCB pointLight[4]; // 点光源配列（最大4灯）
    int pointLightCount; // 有効な点光源の数
    XMFLOAT3 pointLight_dummy; // 16バイト境界合わせ用ダミー
};

//======================================
// 関数プロトタイプ宣言
//======================================

/****************************************
 * @brief ライトシステムの初期化
 * @param[in] pDevice  Direct3Dデバイス
 * @param[in] pContext デバイスコンテキスト
 * @detail
 * 各ライト用の定数バッファを生成し、
 * GPUとの連携準備を行う。
 ****************************************/
void Light_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

/****************************************
 * @brief ライトシステムの終了処理
 * @detail
 * 確保したバッファを解放し、リソースリークを防ぐ。
 ****************************************/
void Light_Finalize(void);

/****************************************
 * @brief 環境光（アンビエントライト）の設定
 * @param[in] color 環境光の色（RGB形式）
 ****************************************/
void Light_SetAmbient(const XMFLOAT3& color);

/****************************************
 * @brief 平行光（ディレクショナルライト）の設定
 * @param[in] world_directional 光の方向ベクトル（ワールド空間）
 * @param[in] color 光の色（RGBA形式）
 ****************************************/
void Light_SetDirectional_World(const XMFLOAT4& world_directional, const XMFLOAT4& color);

/****************************************
 * @brief 鏡面光（スペキュラーライト）の設定
 * @param[in] color           鏡面反射光の色
 * @param[in] CameraPosition  カメラ位置（ワールド座標）
 * @param[in] power           鏡面強度（光沢パワー）
 ****************************************/
void Light_SetSpecular(const XMFLOAT4& color, const XMFLOAT3& CameraPosition, float power);

/****************************************
 * @brief 点光源リストの設定
 * @param[in] lights 点光源配列
 * @param[in] count  有効な点光源数（最大4）
 * @detail
 * 配列の内容をGPU定数バッファに転送し、
 * ピクセルシェーダ側で利用可能にする。
 ****************************************/
void Light_SetPointLight(const PointLightCB* lights, int count);

/****************************************
 * @brief 点光源を1灯追加する
 * @param[in] pos   ワールド座標
 * @param[in] color 発光色
 * @param[in] range 有効距離
 ****************************************/
void Light_AddPointLight(const XMFLOAT3& pos, const XMFLOAT4& color, float range);

/****************************************
 * @brief 個別点光源の座標を更新
 ****************************************/
void Light_SetPointLightPosition(int index, const XMFLOAT3& pos);

/****************************************
 * @brief 個別点光源の色を更新
 ****************************************/
void Light_SetPointLightColor(int index, const XMFLOAT4& color);

/****************************************
 * @brief 個別点光源の範囲を更新
 ****************************************/
void Light_SetPointLightRange(int index, float range);

/****************************************
 * @brief 点光源リストをクリア（全削除）
 ****************************************/
void Light_ClearPointLights(void);

/****************************************
 * @brief 全ライト情報をGPUへ一括転送
 * @detail
 * 毎フレームの描画直前に呼び出し、
 * 内部キャッシュ状態をGPUへ反映する。
 ****************************************/
void Light_Update(double elapsed_time);


#endif // LIGHT_H
