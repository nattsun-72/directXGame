/****************************************
 * @file sound_manager.cpp
 * @brief ゲーム全体のサウンド管理
 * @author Natsume Shidara
 * @date 2026/01/13
 *
 * @note 音源ファイルを追加する場合は SOUND_FILES 配列の
 *       パスを変更してください
 ****************************************/

#include "sound_manager.h"
#include "audio.h"
#include <cmath>

 //======================================
 // 音源ファイルパス設定
 // ※ここを変更すれば音源を差し替え可能
 //======================================
namespace
{
    struct SoundFileInfo
    {
        const char* path;
        bool isBGM;
    };

    const SoundFileInfo SOUND_FILES[SOUND_MAX] =
    {
        // BGM
        { "assets/sound/bgm/title.mp3",         true  },  // SOUND_BGM_TITLE
        { "assets/sound/bgm/game.mp3",          true  },  // SOUND_BGM_GAME
        { "assets/sound/bgm/result.mp3",        true  },  // SOUND_BGM_RESULT

        // SE - プレイヤー
        { "assets/sound/se/slash.mp3",          false },  // SOUND_SE_SLASH
        { "assets/sound/se/slash_heavy.mp3",    false },  // SOUND_SE_SLASH_HEAVY
        { "assets/sound/se/slash_hit.mp3",      false },  // SOUND_SE_SLASH_HIT
        { "assets/sound/se/player_damage.mp3",  false },  // SOUND_SE_PLAYER_DAMAGE
        { "assets/sound/se/jump.mp3",           false },  // SOUND_SE_JUMP
        { "assets/sound/se/double_jump.mp3",    false },  // SOUND_SE_DOUBLE_JUMP
        { "assets/sound/se/dash_charge.mp3",    false },  // SOUND_SE_DASH_CHARGE
        { "assets/sound/se/dash.mp3",           false },  // SOUND_SE_DASH
        { "assets/sound/se/step.mp3",           false },  // SOUND_SE_STEP
        { "assets/sound/se/land.mp3",           false },  // SOUND_SE_LAND

        // SE - コンボ
        { "assets/sound/se/combo_1.mp3",        false },  // SOUND_SE_COMBO_1
        { "assets/sound/se/combo_2.mp3",        false },  // SOUND_SE_COMBO_2
        { "assets/sound/se/combo_3.mp3",        false },  // SOUND_SE_COMBO_3
        { "assets/sound/se/combo_4.mp3",        false },  // SOUND_SE_COMBO_4

        // SE - 敵（地上型）
        { "assets/sound/se/enemy_fire.mp3",     false },  // SOUND_SE_ENEMY_FIRE
        { "assets/sound/se/enemy_death.mp3",    false },  // SOUND_SE_ENEMY_DEATH
        { "assets/sound/se/enemy_slice.mp3",    false },  // SOUND_SE_ENEMY_SLICE

        // SE - 敵（飛行型）
        { "assets/sound/se/flying_charge_ready.mp3", false },  // SOUND_SE_FLYING_CHARGE_READY
        { "assets/sound/se/flying_charge.mp3",       false },  // SOUND_SE_FLYING_CHARGE
        { "assets/sound/se/flying_whoosh.mp3",       false },  // SOUND_SE_FLYING_WHOOSH

        // SE - UI
        { "assets/sound/se/select.mp3",         false },  // SOUND_SE_SELECT
        { "assets/sound/se/decide.mp3",         false },  // SOUND_SE_DECIDE
        { "assets/sound/se/cancel.mp3",         false },  // SOUND_SE_CANCEL
        { "assets/sound/se/pause.mp3",          false },  // SOUND_SE_PAUSE

        // SE - タイトル
        { "assets/sound/se/title_slash.mp3",    false },  // SOUND_SE_TITLE_SLASH

        // SE - リザルト
        { "assets/sound/se/score_count.mp3",    false },  // SOUND_SE_SCORE_COUNT
        { "assets/sound/se/score_finish.mp3",   false },  // SOUND_SE_SCORE_FINISH
    };
}

//======================================
// 内部変数
//======================================
static int g_AudioHandles[SOUND_MAX];
static SoundID g_CurrentBGM = SOUND_MAX;
static float g_BGMVolume = 1.0f;
static float g_SEVolume = 1.0f;

//======================================
// 初期化
//======================================
void SoundManager_Initialize()
{
    InitAudio();

    for (int i = 0; i < SOUND_MAX; i++)
    {
        if (SOUND_FILES[i].path != nullptr)
        {
            g_AudioHandles[i] = LoadAudio(SOUND_FILES[i].path);
        }
        else
        {
            g_AudioHandles[i] = -1;
        }
    }

    g_CurrentBGM = SOUND_MAX;
    g_BGMVolume = 1.0f;
    g_SEVolume = 1.0f;
}

//======================================
// 終了処理
//======================================
void SoundManager_Finalize()
{
    for (int i = 0; i < SOUND_MAX; i++)
    {
        if (g_AudioHandles[i] >= 0)
        {
            UnloadAudio(g_AudioHandles[i]);
            g_AudioHandles[i] = -1;
        }
    }

    UninitAudio();
}

//======================================
// BGM再生
//======================================
void SoundManager_PlayBGM(SoundID id)
{
    if (id < 0 || id >= SOUND_MAX) return;
    if (g_AudioHandles[id] < 0) return;

    if (g_CurrentBGM == id) return;

    SoundManager_StopBGM();

    // ボリュームを設定してから再生
    SetAudioVolume(g_AudioHandles[id], g_BGMVolume);
    PlayAudio(g_AudioHandles[id], true);
    g_CurrentBGM = id;
}

//======================================
// BGM停止
//======================================
void SoundManager_StopBGM()
{
    if (g_CurrentBGM != SOUND_MAX && g_AudioHandles[g_CurrentBGM] >= 0)
    {
        StopAudio(g_AudioHandles[g_CurrentBGM]);
        g_CurrentBGM = SOUND_MAX;
    }
}

//======================================
// BGM音量設定
//======================================
void SoundManager_SetBGMVolume(float volume)
{
    g_BGMVolume = fmaxf(0.0f, fminf(1.0f, volume));

    // 現在再生中のBGMに即座に適用
    if (g_CurrentBGM != SOUND_MAX && g_AudioHandles[g_CurrentBGM] >= 0)
    {
        SetAudioVolume(g_AudioHandles[g_CurrentBGM], g_BGMVolume);
    }
}

//======================================
// SE再生
//======================================
void SoundManager_PlaySE(SoundID id)
{
    if (id < 0 || id >= SOUND_MAX) return;
    if (g_AudioHandles[id] < 0) return;

    // ボリュームを設定してから再生
    SetAudioVolume(g_AudioHandles[id], g_SEVolume);
    PlayAudio(g_AudioHandles[id], false);
}

//======================================
// SE音量設定
//======================================
void SoundManager_SetSEVolume(float volume)
{
    g_SEVolume = fmaxf(0.0f, fminf(1.0f, volume));
}

//======================================
// コンボ数に応じたSE再生
//======================================
void SoundManager_PlayComboSE(int comboCount)
{
    SoundID id;

    if (comboCount >= 50)
    {
        id = SOUND_SE_COMBO_4;
    }
    else if (comboCount >= 20)
    {
        id = SOUND_SE_COMBO_3;
    }
    else if (comboCount >= 10)
    {
        id = SOUND_SE_COMBO_2;
    }
    else
    {
        id = SOUND_SE_COMBO_1;
    }

    SoundManager_PlaySE(id);
}

//======================================
// 全停止
//======================================
void SoundManager_StopAll()
{
    // 全オーディオを停止
    StopAllAudio();

    // BGM状態をリセット
    g_CurrentBGM = SOUND_MAX;
}