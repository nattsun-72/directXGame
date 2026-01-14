/**
 * @file sprite_anim.h
 * @brief スプライトアニメーション描画
 * @author Natsume Shidara
 * @date 2025/06/17
 */

#ifndef SPRITE_ANIM_H
#define SPRITE_ANIM_H

#include <DirectXMath.h>

void SpriteAnim_Initialize();
void SpriteAnim_Finalize(void);

void SpriteAnim_Update(double elapsed_time);
void SpriteAnim_Draw(int playid, float dx, float dy, float dw, float dh);


void BillboardAnim_Draw(int playId, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT2& pivot = {0.0f,0.0f}); 


/**
 * 
 * @param textureId 管理番号
 * @param patternMax パターンの画像数
 * @param m_HPatternMax 一列（横）のパターン最大数
 * @param m_seconds_per_pattern 再生速度、１フレームかかる時間
 * @param patternSize パターン一個のサイズ
 * @param patternStartPosition 最初のパターンの左上座標
 * @param isLoop ループするか
 * @return
 *
 * @pre 必ずTexture_Load()のあと
 */
int SpriteAnim_RegisterPattern(
    int textureId,
    int patternMax,
    int m_HPatternMax,
    double m_seconds_per_pattern,
    DirectX::XMUINT2 patternSize,
    DirectX::XMUINT2 patternStartPosition,
    bool isLoop = true
);

int SpriteAnim_CreatePlayer(int anim_pattern_id);
bool SpriteAnim_IsStopped(int index);
void SpriteAnim_DestroyPlayer(int index);

#endif
