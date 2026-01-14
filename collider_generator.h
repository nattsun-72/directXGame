/****************************************
 * @file collider_generator.h
 * @brief コライダー自動生成ヘルパー
 * @author Natsume Shidara
 * @date 2025/12/15
 * @update 2025/12/15
 ****************************************/

#ifndef COLLIDER_GENERATOR_H
#define COLLIDER_GENERATOR_H

#include "model.h"
#include "collider.h"

 //--------------------------------------
 // クラス宣言
 //--------------------------------------
 /****************************************
  * @class ColliderGenerator
  * @brief モデル形状から最適なコライダーを生成する静的クラス
  *
  * モデルの全頂点座標を解析し、最もフィットする
  * OBB（有向境界ボックス）を算出します。
  ****************************************/
class ColliderGenerator
{
public:
    //======================================
    // 公開関数
    //======================================
    /**
     * @brief モデルに最適なコライダー(OBB)を生成
     * @param pModel 解析対象のモデルデータ
     * @return 生成されたコライダー構造体
     */
    static Collider GenerateBestFit(const MODEL* pModel, ColliderType colliderType = ColliderType::Box);

private:
    // インスタンス化禁止
    ColliderGenerator() = delete;
    ~ColliderGenerator() = delete;
};

#endif // COLLIDER_GENERATOR_H