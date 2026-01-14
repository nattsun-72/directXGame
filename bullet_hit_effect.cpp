#include "bullet_hit_effect.h"
#include "DirectXMath.h"
#include "sprite_anim.h"
#include "texture.h"
#include "direct3d.h"

using namespace DirectX;

int g_TexId = -1;
int g_AnimPatternId = -1;
int g_EffectsCount = 0;

class BulletHitEffect
{
private:
    XMFLOAT3 m_position{};
    int m_anim_play_id = -1;
    bool m_is_destroy{ false };

public:
    BulletHitEffect(const XMFLOAT3& position) : m_position(position), m_anim_play_id(SpriteAnim_CreatePlayer(g_AnimPatternId)) {}

    ~BulletHitEffect() { SpriteAnim_DestroyPlayer(m_anim_play_id); }

    void Update();
    void Draw() const;
    bool IsDestroy() const { return m_is_destroy; }
};

static constexpr int EFFECT_MAX = 256;
static BulletHitEffect* g_pEffects[EFFECT_MAX]{};

void BulletHItEffect_Initialize()
{
    g_TexId = Texture_Load(L"assets/Explosion.png");
    g_AnimPatternId = SpriteAnim_RegisterPattern(g_TexId, 16, 4, 0.01, { 256, 256 }, { 0, 0 }, false);
    g_EffectsCount = 0; // 初期化を明示的に
}

void BulletHItEffect_Finalize(void)
{
    for (int i = 0; i < g_EffectsCount; i++)
    {
        delete g_pEffects[i];
        g_pEffects[i] = nullptr; // 念のためnullptrに
    }
    g_EffectsCount = 0;
}

void BulletHItEffect_Update(double elapsed_time)
{
    (void)elapsed_time;
    // 全エフェクトを更新
    for (int i = 0; i < g_EffectsCount; i++)
    {
        g_pEffects[i]->Update();
    }

    // 破棄フラグが立ったものを削除
    for (int i = g_EffectsCount - 1; i >= 0; i--)
    {
        if (g_pEffects[i]->IsDestroy())
        {
            delete g_pEffects[i];
            g_pEffects[i] = g_pEffects[g_EffectsCount - 1];
            g_pEffects[g_EffectsCount - 1] = nullptr; // 念のため
            g_EffectsCount--;
        }
    }
}

void BulletHItEffect_Create(const DirectX::XMFLOAT3 position)
{
    // 配列の上限チェック
    if (g_EffectsCount >= EFFECT_MAX)
        return;

    // ヒープに確保（修正点）
    g_pEffects[g_EffectsCount++] = new BulletHitEffect(position);
}

void BulletHItEffect_Draw()
{
    Direct3D_SetDepthWriteEnable(false);
    for (int i = 0; i < g_EffectsCount; i++)
    {
        g_pEffects[i]->Draw();
    }
    Direct3D_SetDepthWriteEnable(true);
}

void BulletHitEffect::Update()
{
    if (SpriteAnim_IsStopped(m_anim_play_id))
    {
        m_is_destroy = true;
    }
}

void BulletHitEffect::Draw() const { BillboardAnim_Draw(m_anim_play_id, m_position, { 1.0f, 1.0f }); }