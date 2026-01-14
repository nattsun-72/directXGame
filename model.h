/****************************************
 * @file model.h
 * @brief モデル定義・描画管理
 * @author Natsume Shidara
 * @update 2025/12/12
 ****************************************/

#ifndef MODEL_H
#define MODEL_H

 //--------------------------------------
 // インクルードガード／依存ヘッダ
 //--------------------------------------
#include <unordered_map>
#include <vector>
#include <string>

#include <d3d11.h>
#include <DirectXMath.h>

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/matrix4x4.h"

// ライブラリリンク指定
#pragma comment (lib, "assimp-vc143-mt.lib")

#include "collision.h" // AABB定義

//--------------------------------------
// 共通データ定義
//--------------------------------------
/**
 * @struct Vertex
 * @brief 頂点データ構造
 */
struct Vertex
{
    DirectX::XMFLOAT3 position; // 位置
    DirectX::XMFLOAT3 normal;   // 法線
    DirectX::XMFLOAT4 color;    // カラー
    DirectX::XMFLOAT2 uv;       // UV
};

/**
 * @struct MeshData
 * @brief メッシュごとのCPU側データ（切断計算・加工用）
 */
struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int materialIndex = 0;
};

/**
 * @struct MODEL
 * @brief モデル管理構造体
 * @detail Assimpのシーンデータ、GPUリソース、および加工用CPUデータを保持
 * 参照カウント方式によりリソース管理を行う
 */
struct MODEL
{
    const aiScene* AiScene = nullptr;

    // GPUリソース
    ID3D11Buffer** VertexBuffer = nullptr; // メッシュ数分確保
    ID3D11Buffer** IndexBuffer = nullptr;  // メッシュ数分確保

    // テクスチャリソース
    std::unordered_map<std::string, ID3D11ShaderResourceView*> Texture;
    
    ID3D11ShaderResourceView* pSliceTextureSRV = nullptr;
    int sliceTextureId = -1;

    // CPU側データ（切断・衝突判定用）
    std::vector<MeshData> Meshes;

    // バウンディングボックス
    AABB local_aabb{};

    // 管理用情報
    unsigned int meshCount = 0; // メッシュ数管理
    std::vector<ID3D11ShaderResourceView*> materials; // マテリアル管理用

    // リソース管理用追加メンバ
    std::string resourceKey; // キャッシュキー（ファイルパス）。空文字なら動的生成モデル。
    int refCount = 1;        // 参照カウンタ（初期値1）

    void SetSlicedTexturId(int texId){ sliceTextureId = texId;}
    const int& GetSlicedTexturId() const {return sliceTextureId;}
};

//--------------------------------------
// 関数プロトタイプ宣言
//--------------------------------------

/**
 * @brief モデルファイルの読み込み（キャッシュ対応）
 * @detail 既に読み込まれているファイルはキャッシュから返し、参照カウントを増やす
 * @param FileName ファイルパス
 * @param scale スケール（デフォルト 1.0f）
 * @param RHFlg 右手座標系フラグ（デフォルト false）
 * @return 生成または取得されたMODEL構造体へのポインタ
 */
MODEL* ModelLoad(const char* FileName, float scale = 1.0f, bool RHFlg = false, const wchar_t* pSlisedTextureFilename = nullptr);

/**
 * @brief モデルの参照カウントを増やす
 * @detail モデルポインタを共有する場合に必ず呼び出すこと
 * @param model 対象モデル
 */
void ModelAddRef(MODEL* model);

/**
 * @brief モデルリソースの解放（参照カウント減算）
 * @detail カウントが0になった場合のみメモリから削除される
 * @param model 解放するモデルポインタ
 */
void ModelRelease(MODEL* model);

/**
 * @brief モデルの描画
 * @param model 描画するモデル
 * @param mtxWorld ワールド行列
 */
void ModelDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld);

/**
 * @brief ライティングなしでのモデル描画（デバッグやUI用）
 * @param pModel 描画するモデル
 * @param mtxWorld ワールド行列
 */
void ModelUnlitDraw(MODEL* pModel, const DirectX::XMMATRIX& mtxWorld);

/**
 * @brief シャドウマップ生成用描画
 * @detail シェーダー切り替えを行わず、ジオメトリのみを描画パイプラインに流す
 * @param pModel 描画するモデル
 * @param mtxWorld ワールド行列
 */
void ModelDrawShadow(MODEL* pModel, const DirectX::XMMATRIX& mtxWorld);

/**
 * @brief モデルの現在位置におけるAABBを取得
 * @param model 対象モデル
 * @param position 現在のワールド座標
 * @return ワールド空間のAABB
 */
AABB Model_GetAABB(MODEL* model, const DirectX::XMFLOAT3& position);

/**
 * @brief 生データからMODEL構造体を作成する（切断後のモデル生成用）
 * @param meshes メッシュデータのリスト
 * @param original 元のモデル（テクスチャなどを引き継ぐため）
 * @return 新しいMODELポインタ（キャッシュ管理外・refCount=1）
 */
MODEL* ModelCreateFromData(const std::vector<MeshData>& meshes, MODEL* original);

void ModelDrawDebug(MODEL* model, const DirectX::XMFLOAT3& position);
#endif // MODEL_H