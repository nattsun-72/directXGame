/****************************************
 * @file model.cpp
 * @brief FBX/OBJなどの3Dモデル読み込み・描画管理クラス
 * - Assimpによるモデルロード
 * - リソースキャッシュ機能（参照カウント）
 * - メッシュ切断用CPUデータ保持
 * @author Natsume Shidara
 * @update 2025/12/12
 ****************************************/

#include "model.h"
#include "WICTextureLoader11.h"
#include "direct3d.h"
#include "texture.h"
#include "shader3d.h"
#include "shader3d_unlit.h"
#include "shader_shadow_map.h"
#include "debug_ostream.h"
#include "debug_renderer.h"

#include <DirectXMath.h>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cassert>
#include <unordered_map>

using namespace DirectX;

//--------------------------------------
// 内部変数・定数
//--------------------------------------
int g_TextureWhite = -1; // Texture管理側のID
static ID3D11ShaderResourceView* g_pWhiteSRV = nullptr; // 内部フォールバック用SRV

// モデルキャッシュ（ファイルパス -> モデルポインタ）
static std::unordered_map<std::string, MODEL*> g_ModelCache;

//======================================
// モデル読み込み関数
//======================================
MODEL* ModelLoad(const char* FileName, float scale, bool RHFlg, const wchar_t* pSlisedTextureFilename)
{
    
    // キャッシュキーを作成（シンプルにファイルパスとする）
    std::string key = FileName;

    // 1. キャッシュ確認
    auto it = g_ModelCache.find(key);
    if (it != g_ModelCache.end())
    {
        // ヒットしたら参照カウントを増やして既存のポインタを返す
        ModelAddRef(it->second);
        return it->second;
    }

    // 2. 新規ロード
    MODEL* model = new MODEL;
    model->resourceKey = key; // キーを記憶
    model->refCount = 1;      // 初期カウント


    if (pSlisedTextureFilename != nullptr) {
        // 1. UV計算用にTextureシステム経由でIDを取得（既存通り）
        model->SetSlicedTexturId(Texture_Load(pSlisedTextureFilename));

        // 2. 描画用にSRVを直接作成して保持する
        ID3D11Resource* pTex = nullptr;
        HRESULT hr = CreateWICTextureFromFile(Direct3D_GetDevice(), pSlisedTextureFilename, &pTex, &model->pSliceTextureSRV);
        if (SUCCEEDED(hr)) {
            SAFE_RELEASE(pTex); // テクスチャリソース自体はReleaseしてOK（SRVが持ってるので）
        }
    }
    else {
        model->SetSlicedTexturId(-1);
        model->pSliceTextureSRV = nullptr;
    }

    // パス情報の構築
    std::filesystem::path fbxPath(FileName);
    std::filesystem::path directoryPath = fbxPath.parent_path();
    std::string directory = directoryPath.string();
    std::filesystem::path texDirectory = directoryPath / "TEX";

    // Assimpインポート設定
    unsigned int importFlags = aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_GenUVCoords |
        aiProcess_ImproveCacheLocality |
        aiProcess_OptimizeMeshes |
        aiProcess_ConvertToLeftHanded |
        aiProcess_SortByPType;

    model->AiScene = aiImportFile(FileName, importFlags);
    if (!model->AiScene)
    {
        std::string msg = "ModelLoad Error: Failed to load file: ";
        msg += FileName;
        msg += "\n";
        OutputDebugStringA(msg.c_str());
        MessageBoxA(nullptr, msg.c_str(), "ModelLoad Error", MB_OK | MB_ICONERROR);
        delete model;
        return nullptr;
    }

    // メッシュデータの構築
    model->meshCount = model->AiScene->mNumMeshes;
    model->VertexBuffer = new ID3D11Buffer * [model->meshCount];
    model->IndexBuffer = new ID3D11Buffer * [model->meshCount];
    model->Meshes.resize(model->meshCount);

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        aiMesh* mesh = model->AiScene->mMeshes[m];
        MeshData& meshData = model->Meshes[m];
        meshData.materialIndex = mesh->mMaterialIndex;

        // 頂点データ抽出
        meshData.vertices.resize(mesh->mNumVertices);
        for (unsigned int v = 0; v < mesh->mNumVertices; v++)
        {
            Vertex& vert = meshData.vertices[v];
            if (RHFlg)
            {
                vert.position = XMFLOAT3(mesh->mVertices[v].x * scale, -mesh->mVertices[v].z * scale, mesh->mVertices[v].y * scale);
                vert.normal = XMFLOAT3(mesh->mNormals[v].x, -mesh->mNormals[v].z, mesh->mNormals[v].y);
            }
            else
            {
                vert.position = XMFLOAT3(mesh->mVertices[v].x * scale, mesh->mVertices[v].y * scale, mesh->mVertices[v].z * scale);
                vert.normal = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
            }

            if (mesh->mTextureCoords[0])
                vert.uv = XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
            else
                vert.uv = XMFLOAT2(0.0f, 0.0f);

            if (mesh->HasVertexColors(0))
                vert.color = XMFLOAT4(mesh->mColors[0][v].r, mesh->mColors[0][v].g, mesh->mColors[0][v].b, mesh->mColors[0][v].a);
            else
                vert.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

            // AABB更新
            if (v == 0 && m == 0)
            {
                model->local_aabb.min = vert.position;
                model->local_aabb.max = vert.position;
            }
            else
            {
                model->local_aabb.min.x = std::fminf(model->local_aabb.min.x, vert.position.x);
                model->local_aabb.min.y = std::fminf(model->local_aabb.min.y, vert.position.y);
                model->local_aabb.min.z = std::fminf(model->local_aabb.min.z, vert.position.z);
                model->local_aabb.max.x = std::fmaxf(model->local_aabb.max.x, vert.position.x);
                model->local_aabb.max.y = std::fmaxf(model->local_aabb.max.y, vert.position.y);
                model->local_aabb.max.z = std::fmaxf(model->local_aabb.max.z, vert.position.z);
            }
        }

        // インデックスデータ
        for (unsigned int f = 0; f < mesh->mNumFaces; f++)
        {
            const aiFace* face = &mesh->mFaces[f];
            if (face->mNumIndices == 3)
            {
                meshData.indices.push_back(face->mIndices[0]);
                meshData.indices.push_back(face->mIndices[1]);
                meshData.indices.push_back(face->mIndices[2]);
            }
        }

        // GPUバッファ
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(Vertex) * static_cast<UINT>(meshData.vertices.size());
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = meshData.vertices.data();
        Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);

        bd.ByteWidth = sizeof(unsigned int) * static_cast<UINT>(meshData.indices.size());
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        sd.pSysMem = meshData.indices.data();
        Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);
    }

    // テクスチャ読み込み
    if (g_TextureWhite == -1)
    {
        g_TextureWhite = Texture_Load(L"assets/white.png");
        CreateWICTextureFromFile(Direct3D_GetDevice(), L"assets/white.png", nullptr, &g_pWhiteSRV);
    }

    model->materials.resize(model->AiScene->mNumMaterials, nullptr);

    for (unsigned int m = 0; m < model->AiScene->mNumMaterials; m++)
    {
        aiMaterial* pMat = model->AiScene->mMaterials[m];
        aiString texPath;

        if (pMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) != AI_SUCCESS)
        {
            pMat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath);
        }

        if (texPath.length == 0)
        {
            model->materials[m] = g_pWhiteSRV;
            if (g_pWhiteSRV) g_pWhiteSRV->AddRef();
            continue;
        }

        const aiTexture* pEmbeddedTex = model->AiScene->GetEmbeddedTexture(texPath.C_Str());
        if (pEmbeddedTex)
        {
            // 埋め込みテクスチャ
            ID3D11Resource* pTex = nullptr;
            ID3D11ShaderResourceView* pSRV = nullptr;
            size_t size = (pEmbeddedTex->mHeight == 0) ? pEmbeddedTex->mWidth : pEmbeddedTex->mWidth * pEmbeddedTex->mHeight * 4;
            HRESULT hr = CreateWICTextureFromMemory(Direct3D_GetDevice(), reinterpret_cast<const uint8_t*>(pEmbeddedTex->pcData), size, &pTex, &pSRV);

            if (SUCCEEDED(hr))
            {
                if (pTex) pTex->Release();
                model->materials[m] = pSRV;
                std::string texKey = texPath.C_Str();
                if (model->Texture.find(texKey) == model->Texture.end())
                {
                    model->Texture[texKey] = pSRV;
                    pSRV->AddRef();
                }
            }
            else
            {
                model->materials[m] = g_pWhiteSRV;
                if (g_pWhiteSRV) g_pWhiteSRV->AddRef();
            }
            continue;
        }

        // 外部ファイル読み込み
        std::string filenameOnly = std::filesystem::path(texPath.C_Str()).filename().string();
        if (model->Texture.count(filenameOnly))
        {
            model->materials[m] = model->Texture[filenameOnly];
            model->materials[m]->AddRef();
            continue;
        }

        std::filesystem::path targetPath;
        bool found = false;

        targetPath = texDirectory / filenameOnly;
        if (std::filesystem::exists(targetPath)) found = true;

        if (!found)
        {
            targetPath = std::filesystem::path(directory) / filenameOnly;
            if (std::filesystem::exists(targetPath)) found = true;
        }
        if (!found)
        {
            targetPath = std::filesystem::path(directory) / texPath.C_Str();
            if (std::filesystem::exists(targetPath)) found = true;
        }
        if (!found)
        {
            targetPath = texPath.C_Str();
            if (std::filesystem::exists(targetPath)) found = true;
        }

        if (found)
        {
            ID3D11Resource* pTex = nullptr;
            ID3D11ShaderResourceView* pSRV = nullptr;
            HRESULT hr = CreateWICTextureFromFile(Direct3D_GetDevice(), targetPath.c_str(), &pTex, &pSRV);

            if (SUCCEEDED(hr))
            {
                if (pTex) pTex->Release();
                model->Texture[filenameOnly] = pSRV;
                model->materials[m] = pSRV;
                pSRV->AddRef();
            }
            else
            {
                model->materials[m] = g_pWhiteSRV;
                if (g_pWhiteSRV) g_pWhiteSRV->AddRef();
            }
        }
        else
        {
            model->materials[m] = g_pWhiteSRV;
            if (g_pWhiteSRV) g_pWhiteSRV->AddRef();
        }
    }

    // 最後にキャッシュへ登録
    g_ModelCache[key] = model;

    return model;
}

//======================================
// データからモデル生成 (切断結果用)
//======================================
MODEL* ModelCreateFromData(const std::vector<MeshData>& meshes, MODEL* original)
{
    MODEL* model = new MODEL;
    model->AiScene = nullptr;

    // 切断されたモデルは独自形状なのでリソースキーは持たない
    model->resourceKey = "";
    model->refCount = 1;

    // マテリアル・テクスチャリソースを継承
    model->materials = original->materials;

    if (original->pSliceTextureSRV)
    {
        model->materials.push_back(original->pSliceTextureSRV);

        // 新しいモデルにも断面情報を引き継ぐ（再切断のため）
        model->pSliceTextureSRV = original->pSliceTextureSRV;
        model->sliceTextureId = original->sliceTextureId; // IDも引き継ぐ
    }
    else
    {
        model->pSliceTextureSRV = nullptr;
        model->sliceTextureId = -1;
    }

    for (auto* srv : model->materials)
    {
        if (srv) srv->AddRef();
    }

    model->Texture = original->Texture;
    for (auto& pair : model->Texture)
    {
        if (pair.second) pair.second->AddRef();
    }

    model->meshCount = static_cast<unsigned int>(meshes.size());
    model->Meshes = meshes;
    model->VertexBuffer = new ID3D11Buffer * [model->meshCount];
    model->IndexBuffer = new ID3D11Buffer * [model->meshCount];

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        const MeshData& data = meshes[m];

        if (data.vertices.empty())
        {
            model->VertexBuffer[m] = nullptr;
            model->IndexBuffer[m] = nullptr;
            continue;
        }

        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(Vertex) * static_cast<UINT>(data.vertices.size());
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = data.vertices.data();
        Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);

        bd.ByteWidth = sizeof(unsigned int) * static_cast<UINT>(data.indices.size());
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        sd.pSysMem = data.indices.data();
        Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);

        for (size_t i = 0; i < data.vertices.size(); i++)
        {
            const XMFLOAT3& p = data.vertices[i].position;
            if (m == 0 && i == 0)
            {
                model->local_aabb.min = p;
                model->local_aabb.max = p;
            }
            else
            {
                model->local_aabb.min.x = std::fminf(model->local_aabb.min.x, p.x);
                model->local_aabb.min.y = std::fminf(model->local_aabb.min.y, p.y);
                model->local_aabb.min.z = std::fminf(model->local_aabb.min.z, p.z);
                model->local_aabb.max.x = std::fmaxf(model->local_aabb.max.x, p.x);
                model->local_aabb.max.y = std::fmaxf(model->local_aabb.max.y, p.y);
                model->local_aabb.max.z = std::fmaxf(model->local_aabb.max.z, p.z);
            }
        }
    }

    return model;
}

//======================================
// 参照カウント増加
//======================================
void ModelAddRef(MODEL* model)
{
    if (model)
    {
        model->refCount++;
    }
}

//======================================
// モデル解放関数 (参照カウント式)
//======================================
void ModelRelease(MODEL* model)
{
    if (!model) return;

    // 1. 参照カウントを減らす
    model->refCount--;

    // 2. まだ誰かが使っているなら、ここで終了（削除しない）
    if (model->refCount > 0)
    {
        return;
    }



    // キャッシュ登録済みなら、マップから削除
    if (!model->resourceKey.empty())
    {
        g_ModelCache.erase(model->resourceKey);
    }

    // GPUバッファ解放
    if (model->VertexBuffer)
    {
        for (unsigned int m = 0; m < model->meshCount; m++)
        {
            SAFE_RELEASE(model->VertexBuffer[m]);
        }
        delete[] model->VertexBuffer;
    }

    if (model->IndexBuffer)
    {
        for (unsigned int m = 0; m < model->meshCount; m++)
        {
            SAFE_RELEASE(model->IndexBuffer[m]);
        }
        delete[] model->IndexBuffer;
    }

    model->pSliceTextureSRV = nullptr;

    // マテリアルSRV解放
    for (auto* srv : model->materials)
    {
        SAFE_RELEASE(srv);
    }
    model->materials.clear();

    // テクスチャマップ解放
    for (auto& tex : model->Texture)
    {
        SAFE_RELEASE(tex.second);
    }
    model->Texture.clear();

    // Assimpシーン破棄
    if (model->AiScene)
    {
        aiReleaseImport(model->AiScene);
        model->AiScene = nullptr;
    }

    delete model;
}

//======================================
// モデル描画関数
//======================================
void ModelDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
    if (!model) return;

    ID3D11DeviceContext* pContext = Direct3D_GetContext();

    Shader3D_Begin();
    Shader3D_SetWorldMatrix(mtxWorld);

    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Direct3D_DepthStencilStateDepthIsEnable(true);

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        if (!model->VertexBuffer[m]) continue;

        unsigned int matIdx = model->Meshes[m].materialIndex;
        ID3D11ShaderResourceView* pSRV = nullptr;

        if (matIdx < model->materials.size())
        {
            pSRV = model->materials[matIdx];
        }

        ID3D11ShaderResourceView* textureToBind = pSRV ? pSRV : g_pWhiteSRV;
        pContext->PSSetShaderResources(0, 1, &textureToBind);
        Shader3D_SetColor({ 1, 1, 1, 1 });

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        pContext->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
        pContext->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

        UINT indexCount = static_cast<UINT>(model->Meshes[m].indices.size());
        pContext->DrawIndexed(indexCount, 0, 0);
    }

    Direct3D_DepthStencilStateDepthIsEnable(false);
}

//======================================
// モデル描画関数 (Unlit)
//======================================
void ModelUnlitDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
    if (!model) return;

    ID3D11DeviceContext* pContext = Direct3D_GetContext();

    Shader3D_Unlit_Begin();
    Shader3D_Unlit_SetWorldMatrix(mtxWorld);

    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Direct3D_DepthStencilStateDepthIsEnable(true);

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        if (!model->VertexBuffer[m]) continue;

        unsigned int matIdx = model->Meshes[m].materialIndex;

        XMFLOAT4 color = { 1, 1, 1, 1 };
        if (model->AiScene && matIdx < model->AiScene->mNumMaterials)
        {
            aiColor3D diffuse(1, 1, 1);
            if (model->AiScene->mMaterials[matIdx]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS)
            {
                color = { diffuse.r, diffuse.g, diffuse.b, 1.0f };
            }
        }
        Shader3D_Unlit_SetColor(color);

        ID3D11ShaderResourceView* pSRV = nullptr;
        if (matIdx < model->materials.size())
        {
            pSRV = model->materials[matIdx];
        }
        ID3D11ShaderResourceView* textureToBind = pSRV ? pSRV : g_pWhiteSRV;
        pContext->PSSetShaderResources(0, 1, &textureToBind);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        pContext->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
        pContext->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

        UINT indexCount = static_cast<UINT>(model->Meshes[m].indices.size());
        pContext->DrawIndexed(indexCount, 0, 0);
    }

    Direct3D_DepthStencilStateDepthIsEnable(false);
}

//======================================
// シャドウマップ描画
//======================================
void ModelDrawShadow(MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{
    if (!model) return;

    ID3D11DeviceContext* pContext = Direct3D_GetContext();
    ShaderShadowMap_SetWorldMatrix(mtxWorld);

    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (unsigned int m = 0; m < model->meshCount; m++)
    {
        if (!model->VertexBuffer[m]) continue;

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        pContext->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
        pContext->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

        UINT indexCount = static_cast<UINT>(model->Meshes[m].indices.size());
        pContext->DrawIndexed(indexCount, 0, 0);
    }
}

//======================================
// AABB取得
//======================================
AABB Model_GetAABB(MODEL* model, const DirectX::XMFLOAT3& position)
{
    if (!model) return AABB();
    return {
        { position.x + model->local_aabb.min.x, position.y + model->local_aabb.min.y, position.z + model->local_aabb.min.z },
        { position.x + model->local_aabb.max.x, position.y + model->local_aabb.max.y, position.z + model->local_aabb.max.z }
    };
}

void ModelDrawDebug(MODEL* model, const DirectX::XMFLOAT3& position)
{
    if (!model) return;

    // 現在の計算式に基づいたAABBを取得
    AABB aabb = Model_GetAABB(model, position);

    // 黄色で描画
    DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 0.0f, 1.0f };
    DebugRenderer::DrawAABB(aabb, color);
}