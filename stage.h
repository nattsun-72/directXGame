/****************************************
 * @file stage.h
 * @brief ステージ管理（地形・壁・スポーンポイント）
 * @author Natsume Shidara
 * @date 2026/01/13
 * @update 2026/01/13 - MeshField統合
 ****************************************/

#ifndef STAGE_H
#define STAGE_H

#include <DirectXMath.h>

 //======================================
 // ステージ設定定数
 //======================================
namespace StageConfig
{
    // ステージサイズ（MeshFieldに合わせる）
    constexpr float STAGE_WIDTH = 150.0f;       // X方向の幅
    constexpr float STAGE_DEPTH = 150.0f;       // Z方向の奥行き
    constexpr float WALL_HEIGHT = 12.0f;       // 壁の高さ
    constexpr float WALL_THICKNESS = 1.5f;     // 壁の厚さ

    // プレイエリア（壁の内側）
    constexpr float PLAY_AREA_MIN_X = -STAGE_WIDTH * 0.5f + WALL_THICKNESS + 1.0f;
    constexpr float PLAY_AREA_MAX_X = STAGE_WIDTH * 0.5f - WALL_THICKNESS - 1.0f;
    constexpr float PLAY_AREA_MIN_Z = -STAGE_DEPTH * 0.5f + WALL_THICKNESS + 1.0f;
    constexpr float PLAY_AREA_MAX_Z = STAGE_DEPTH * 0.5f - WALL_THICKNESS - 1.0f;

    // スポーン設定
    constexpr float SPAWN_HEIGHT_GROUND = 2.0f;   // 地上型敵のスポーン高度（地形より上）
    constexpr float SPAWN_HEIGHT_FLYING = 10.0f;  // 飛行型敵のスポーン高度
    constexpr float SPAWN_MARGIN = 5.0f;          // 壁からのマージン
    constexpr float PLAYER_SAFE_RADIUS = 10.0f;   // プレイヤー周辺のスポーン禁止半径
}

//======================================
// 公開関数
//======================================

/**
 * @brief ステージ初期化
 */
void Stage_Initialize();

/**
 * @brief ステージ終了処理
 */
void Stage_Finalize();

/**
 * @brief ステージ更新
 */
void Stage_Update(float dt);

/**
 * @brief ステージ描画
 */
void Stage_Draw();

/**
 * @brief ステージシャドウ描画
 */
void Stage_DrawShadow();

/**
 * @brief ステージデバッグ描画
 */
void Stage_DrawDebug();

/**
 * @brief 地上型敵のスポーン位置を取得（ランダム）
 * @param playerPos プレイヤーの現在位置（近くにはスポーンしない）
 * @return スポーン位置
 */
DirectX::XMFLOAT3 Stage_GetGroundSpawnPosition(const DirectX::XMFLOAT3& playerPos);

/**
 * @brief 飛行型敵のスポーン位置を取得（ランダム）
 * @param playerPos プレイヤーの現在位置
 * @return スポーン位置
 */
DirectX::XMFLOAT3 Stage_GetFlyingSpawnPosition(const DirectX::XMFLOAT3& playerPos);

/**
 * @brief 位置がプレイエリア内かチェック
 */
bool Stage_IsInsidePlayArea(const DirectX::XMFLOAT3& pos);

/**
 * @brief 位置をプレイエリア内にクランプ
 */
DirectX::XMFLOAT3 Stage_ClampToPlayArea(const DirectX::XMFLOAT3& pos);

/**
 * @brief 指定座標の地形高さを取得
 */
float Stage_GetTerrainHeight(float x, float z);

#endif // STAGE_H