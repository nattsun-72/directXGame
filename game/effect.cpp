#include "effect.h"

#include "audio.h"
#include "DirectXMath.h"
#include "sprite_anim.h"
#include "texture.h"

using namespace DirectX;

struct Effect
{
    XMFLOAT2 position;
    bool isEnable;
    int sprite_anim_id;
};

static constexpr int EFFECT_MAX = 256;
static Effect g_Effects[EFFECT_MAX];

static int g_AnimPatternId = -1;
static int g_EffectTexId = -1;

static int g_EffectSoundId = -1;

void Effect_Initialize()
{
    for (Effect& effect : g_Effects)
    {
        effect.isEnable = false;
    }
    g_EffectTexId = Texture_Load(L"assets/Explosion.png");
    g_AnimPatternId = SpriteAnim_RegisterPattern(g_EffectTexId,
                                                 16, 4, 0.01f,
                                                 {256, 256}, {0, 0}, false);

    g_EffectSoundId = LoadAudio("assets/audio/hit.wav");
}

void Effect_Finalize()
{
    UnloadAudio(g_EffectSoundId);
}

void Effect_Update(double)
{
    for (Effect& effect : g_Effects)
    {
        if (!effect.isEnable) continue;

        if (SpriteAnim_IsStopped(effect.sprite_anim_id))
        {
            SpriteAnim_DestroyPlayer(effect.sprite_anim_id);
            effect.isEnable = false;
        }
    }
}

void Effect_Draw()
{
    for (Effect& effect : g_Effects)
    {
        if (!effect.isEnable) continue;

        SpriteAnim_Draw(effect.sprite_anim_id, effect.position.x, effect.position.y, 64.0f, 64.0f);
    }
}

void Effect_Create(const XMFLOAT2& position)
{
    for (Effect& effect : g_Effects)
    {
        if (effect.isEnable) continue;

        effect.isEnable = true;
        effect.position = position;
        effect.sprite_anim_id = SpriteAnim_CreatePlayer(g_AnimPatternId);

        PlayAudio(g_EffectSoundId, false);

        break;
    }
}
