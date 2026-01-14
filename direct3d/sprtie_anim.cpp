/**
* @file sprite_anim.cpp
 * @brief スプライトアニメーション描画
 * @author Natsume Shidara
 * @date 2025/06/17
 */
#include "sprite_anim.h"
#include "sprite.h"
#include "texture.h"
#include <DirectXMath.h>

#include "debug_ostream.h"

#include "billboard.h"
using namespace DirectX;

struct AnimPatternData
{
    int m_TextureId{1}; ///> テクスチャID @retval -1 登録されていない 
    int m_PatternMax{0}; // パターン数
    int m_HPatternMax{0}; // 一列（横）のパターン最大数
    XMUINT2 m_StartPosition{0, 0}; // アニメーションのスタート座標
    XMUINT2 m_PatternSize{0, 0}; // 1パターンサイズ
    double m_seconds_per_pattern = 0.1;
    bool m_IsLooped{false}; // ループするか
};

struct AnimPlayData
{
    int m_PatternId{-1}; // アニメーションパターンID
    int m_PatternNum{0}; // 現在再生中のパターン番号
    double m_accumulated_time{0}; // 累積時間
    bool m_IsStopped = false;
};

static constexpr int ANIM_PATTERN_MAX = 128;
static AnimPatternData g_AnimPattern[ANIM_PATTERN_MAX];
static constexpr int ANIM_PLAY_MAX = 256;
static AnimPlayData g_AnimPlay[ANIM_PLAY_MAX];

void SpriteAnim_Initialize()
{
    // アニメーションパターン管理情報を初期化（すべて利用していない）状況にする
    for (AnimPatternData& data : g_AnimPattern)
    {
        data.m_TextureId = -1;
    }

    for (AnimPlayData& data : g_AnimPlay)
    {
        data.m_PatternId = -1;
        data.m_IsStopped = false;
    }
}

void SpriteAnim_Finalize(void)
{
}

void SpriteAnim_Update(double elapsed_time)
{
    for (int i = 0; i < ANIM_PLAY_MAX; i++)
    {
        if (g_AnimPlay[i].m_PatternId < 0) continue; // データが入ってない

        const int pattern_id = g_AnimPlay[i].m_PatternId;
        AnimPatternData* pPatternData = &g_AnimPattern[pattern_id];

        if (g_AnimPlay[i].m_accumulated_time >= pPatternData->m_seconds_per_pattern)
        {
            g_AnimPlay[i].m_PatternNum++;

            if (g_AnimPlay[i].m_PatternNum >= pPatternData->m_PatternMax)
            {
                if (pPatternData->m_IsLooped)
                {
                    g_AnimPlay[i].m_PatternNum = 0;
                }
                else
                {
                    g_AnimPlay[i].m_PatternNum = pPatternData->m_PatternMax - 1;
                    g_AnimPlay[i].m_IsStopped = true;
                }
            }

            g_AnimPlay[i].m_accumulated_time -= pPatternData->m_seconds_per_pattern;
        }
        g_AnimPlay[i].m_accumulated_time += elapsed_time;
    }
}

void SpriteAnim_Draw(int playid, float dx, float dy, float dw, float dh)
{
    const int pattern_id = g_AnimPlay[playid].m_PatternId;
    const int pattern_num = g_AnimPlay[playid].m_PatternNum;
    AnimPatternData* pPatternData = &g_AnimPattern[pattern_id];

    Sprite_Draw(
        pPatternData->m_TextureId,
        dx, dy,
        static_cast<float>(pPatternData->m_StartPosition.x + pPatternData->m_PatternSize.x * (pattern_num % pPatternData
            ->m_HPatternMax)),
        static_cast<float>(pPatternData->m_StartPosition.y + pPatternData->m_PatternSize.y * (pattern_num / pPatternData
            ->m_HPatternMax)),
        static_cast<float>(pPatternData->m_PatternSize.x),
        static_cast<float>(pPatternData->m_PatternSize.y),
        dw, dh
    );
}

void BillboardAnim_Draw(int playid, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT2& pivot)
{
    const int pattern_id = g_AnimPlay[playid].m_PatternId;
    const int pattern_num = g_AnimPlay[playid].m_PatternNum;
    AnimPatternData* pPatternData = &g_AnimPattern[pattern_id];

    const XMFLOAT4 texCut = { static_cast<float>(pPatternData->m_StartPosition.x + pPatternData->m_PatternSize.x * (pattern_num % pPatternData->m_HPatternMax)), static_cast<float>(pPatternData->m_StartPosition.y + pPatternData->m_PatternSize.y * (pattern_num / pPatternData->m_HPatternMax)),
                        static_cast<float>(pPatternData->m_PatternSize.x), static_cast<float>(pPatternData->m_PatternSize.y) };

    Billboard_Draw(pPatternData->m_TextureId,position, scale, texCut, pivot);
}

int SpriteAnim_RegisterPattern(int textureId, int patternMax, int m_HPatternMax, double m_seconds_per_pattern,
                               XMUINT2 patternSize,
                               XMUINT2 patternStartPosition, bool isLoop)
{
    for (int i = 0; i < ANIM_PATTERN_MAX; i++)
    {
        if (g_AnimPattern[i].m_TextureId >= 0) continue;

        g_AnimPattern[i].m_TextureId = textureId;
        g_AnimPattern[i].m_PatternMax = patternMax;
        g_AnimPattern[i].m_HPatternMax = m_HPatternMax;
        g_AnimPattern[i].m_seconds_per_pattern = m_seconds_per_pattern;
        g_AnimPattern[i].m_PatternSize = patternSize;
        g_AnimPattern[i].m_StartPosition = patternStartPosition;
        g_AnimPattern[i].m_IsLooped = isLoop;

        return i;
    }

    return -1;
}

int SpriteAnim_CreatePlayer(int anim_pattern_id)
{
    for (int i = 0; i < ANIM_PLAY_MAX; i++)
    {
        if (g_AnimPlay[i].m_PatternId >= 0) continue;

        g_AnimPlay[i].m_PatternId = anim_pattern_id;
        g_AnimPlay[i].m_accumulated_time = 0;
        g_AnimPlay[i].m_PatternNum = 0;

        g_AnimPlay[i].m_IsStopped = false;

        return i;
    }
    return -1;
}

bool SpriteAnim_IsStopped(int index)
{
    return g_AnimPlay[index].m_IsStopped;
}

void SpriteAnim_DestroyPlayer(int index)
{
    g_AnimPlay[index].m_PatternId = -1;
}
