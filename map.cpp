/****************************************
 * @file map.cpp
 * @brief マップ管理およびオブジェクト配置
 * @author Natsume Shidara
 * @date 2025/12/19
 * @update 2025/12/19
 ****************************************/

#include "map.h"
#include "cube.h"
#include <DirectXMath.h>
#include <vector>
#include <cmath>

using namespace DirectX;

#include "color.h"
#include "debug_ostream.h"
#include "direct3d.h"
#include "shader3d.h"
#include "texture.h"
#include "meshfield.h"
#include "light.h"
#include "player_camera.h"
#include "model.h"

//--------------------------------------
// グローバル変数群
//--------------------------------------
static int g_CubeTexId = -1;
static std::wstring TEXTURE_PATH = L"assets/box.png"; // 雪っぽいテクスチャがあれば差し替え推奨

static MODEL* g_pRock01{ nullptr };

//--------------------------------------
// マップ配置データ定義
//--------------------------------------
// カマクラを構成するキューブの最大数を見積もり（調整可能）
static std::vector<MapObject> g_vMapObjects;

//======================================
// 内部補助関数
//======================================
/**
 * @brief カマクラ形状のオブジェクト群を生成する
 * @param centerOffset カマクラの中心座標
 */
void CreateKamakura(XMFLOAT3 centerOffset)
{
    // 地面（KindId: 0）
    g_vMapObjects.push_back({
        0,
        { 0.0f, 0.0f, 0.0f },
        { { -50.0f, -1.0f, -50.0f }, { 50.0f, 0.0f, 50.0f } },
        });

    const float PI = 3.14159265f;
    const float radiusBase = 4.0f; // 底面の半径
    const int stages = 5;          // 積み上げる段数
    const float cubeSize = 1.0f;   // キューブの1辺

    for (int s = 0; s < stages; ++s)
    {
        float y = s * cubeSize;
        // 上に行くほど半径を小さくする（ドーム状にするための簡易計算）
        float currentRadius = radiusBase * cosf((static_cast<float>(s) / stages) * (PI / 2.0f));

        // 段ごとの円周上に配置する個数
        int count = static_cast<int>(2.0f * PI * currentRadius / cubeSize);
        if (count < 1) count = 1;

        for (int i = 0; i < count; ++i)
        {
            float angle = (2.0f * PI / count) * i;

            // 入口を作るために、特定の角度（手前側）はスキップする
            // 角度 1.2π ～ 1.8π あたりを入り口とする例
            if (s < 3 && (angle > PI * 1.2f && angle < PI * 1.8f))
            {
                continue;
            }

            XMFLOAT3 pos;
            pos.x = centerOffset.x + currentRadius * cosf(angle);
            pos.y = centerOffset.y + y + (cubeSize / 2.0f); // 接地面を考慮
            pos.z = centerOffset.z + currentRadius * sinf(angle);

            g_vMapObjects.push_back({ 1, pos });
        }
    }

    // 天井を塞ぐための中心キューブ
    g_vMapObjects.push_back({ 1, { centerOffset.x, centerOffset.y + (stages * cubeSize), centerOffset.z } });
}

//======================================
// 基本動作関数群
//======================================

/**
 * @brief マップの初期化処理
 */
void Map_Initialize()
{
    // テクスチャ読み込み
    g_CubeTexId = Texture_Load(TEXTURE_PATH.c_str());

    // カマクラの生成（中心を原点に設定）
    CreateKamakura({ 0.0f, 0.0f, 0.0f });

    // AABB（当たり判定用ボックス）の更新
    for (MapObject& o : g_vMapObjects)
    {
        if (o.KindId == 1 || o.KindId == 2)
        {
            o.Aabb = Cube_GetAABB(o.Posision);
        }
        else if (o.KindId == 3)
        {
            if (g_pRock01) {
                o.Aabb = Model_GetAABB(g_pRock01, o.Posision);
            }
        }
    }
}

/**
 * @brief マップの解放処理
 */
void Map_Finalize()
{
    if (g_pRock01) {
        ModelRelease(g_pRock01);
        g_pRock01 = nullptr;
    }
    g_vMapObjects.clear();
}

/**
 * @brief マップの更新処理
 */
void Map_Update(double elapsed_time)
{
    (void)elapsed_time;
}

/**
 * @brief マップの描画処理
 */
void Map_Draw()
{
    XMMATRIX mtxWorld = XMMatrixIdentity();

    for (const MapObject& o : g_vMapObjects)
    {
        switch (o.KindId)
        {
        case 0: // MeshField
            Light_SetSpecular({ 0.5f, 0.5f, 0.25f, 1.0f }, PLCamera_GetPosition(), 50.0f);
            MeshField_Draw(mtxWorld);
            break;

        case 1: // Cube
        case 2:
            mtxWorld = XMMatrixTranslation(o.Posision.x, o.Posision.y, o.Posision.z);
            Cube_Draw(g_CubeTexId, mtxWorld);
            break;

        case 3: // Model
            if (g_pRock01) {
                mtxWorld = XMMatrixTranslation(o.Posision.x, o.Posision.y, o.Posision.z);
                ModelDraw(g_pRock01, mtxWorld);
            }
            break;

        default:
            break;
        }
    }
}

/**
 * @brief シャドウマップ生成用描画
 */
void Map_DrawShadow()
{
    XMMATRIX mtxWorld = XMMatrixIdentity();

    for (const MapObject& o : g_vMapObjects)
    {
        switch (o.KindId)
        {
        case 1:
        case 2:
            mtxWorld = XMMatrixTranslation(o.Posision.x, o.Posision.y, o.Posision.z);
            Cube_DrawShadow(mtxWorld);
            break;

        case 3:
            if (g_pRock01) {
                mtxWorld = XMMatrixTranslation(o.Posision.x, o.Posision.y, o.Posision.z);
                ModelDrawShadow(g_pRock01, mtxWorld);
            }
            break;

        default:
            break;
        }
    }
}

/**
 * @brief 指定したインデックスのオブジェクトを取得
 */
const MapObject* Map_GetObject(int index)
{
    if (index < 0 || index >= g_vMapObjects.size()) return nullptr;
    return &g_vMapObjects[index];
}

/**
 * @brief オブジェクト総数を取得
 */
int Map_GetObjectCount()
{
    return static_cast<int>(g_vMapObjects.size());
}