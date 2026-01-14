/****************************************
 * @file sound_manager.h
 * @brief ゲーム全体のサウンド管理
 * @author Natsume Shidara
 * @date 2026/01/13
 ****************************************/

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

 //======================================
 // サウンドID定義
 //======================================
enum SoundID
{
    // BGM
    SOUND_BGM_TITLE,        // タイトル画面
    SOUND_BGM_GAME,         // ゲーム中
    SOUND_BGM_RESULT,       // リザルト画面

    // SE - プレイヤー
    SOUND_SE_SLASH,         // 斬撃（通常）
    SOUND_SE_SLASH_HEAVY,   // 斬撃（強攻撃）
    SOUND_SE_SLASH_HIT,     // 斬撃ヒット
    SOUND_SE_PLAYER_DAMAGE, // プレイヤーダメージ
    SOUND_SE_JUMP,          // ジャンプ
    SOUND_SE_DOUBLE_JUMP,   // 二段ジャンプ
    SOUND_SE_DASH_CHARGE,   // エアダッシュ溜め
    SOUND_SE_DASH,          // エアダッシュ
    SOUND_SE_STEP,          // ステップ
    SOUND_SE_LAND,          // 着地

    // SE - コンボ
    SOUND_SE_COMBO_1,       // コンボ（低）1-9
    SOUND_SE_COMBO_2,       // コンボ（中）10-19
    SOUND_SE_COMBO_3,       // コンボ（高）20-49
    SOUND_SE_COMBO_4,       // コンボ（最高）50+

    // SE - 敵（地上型）
    SOUND_SE_ENEMY_FIRE,    // 敵の発射音
    SOUND_SE_ENEMY_DEATH,   // 敵撃破
    SOUND_SE_ENEMY_SLICE,   // 敵切断

    // SE - 敵（飛行型）
    SOUND_SE_FLYING_CHARGE_READY,  // 飛行敵：突撃準備
    SOUND_SE_FLYING_CHARGE,        // 飛行敵：突撃
    SOUND_SE_FLYING_WHOOSH,        // 飛行敵：風切り音

    // SE - UI
    SOUND_SE_SELECT,        // 選択音
    SOUND_SE_DECIDE,        // 決定音
    SOUND_SE_CANCEL,        // キャンセル音
    SOUND_SE_PAUSE,         // ポーズ

    // SE - タイトル
    SOUND_SE_TITLE_SLASH,   // タイトル画面の斬撃演出

    // SE - リザルト
    SOUND_SE_SCORE_COUNT,   // スコアカウントアップ
    SOUND_SE_SCORE_FINISH,  // スコア表示完了

    // 総数
    SOUND_MAX
};

//======================================
// 初期化・終了
//======================================
void SoundManager_Initialize();
void SoundManager_Finalize();

//======================================
// BGM制御
//======================================
void SoundManager_PlayBGM(SoundID id);
void SoundManager_StopBGM();
void SoundManager_SetBGMVolume(float volume);  // 0.0 ~ 1.0

//======================================
// SE再生
//======================================
void SoundManager_PlaySE(SoundID id);
void SoundManager_SetSEVolume(float volume);   // 0.0 ~ 1.0

//======================================
// コンボ用（コンボ数に応じた音を自動選択）
//======================================
void SoundManager_PlayComboSE(int comboCount);

//======================================
// 一括停止
//======================================
void SoundManager_StopAll();

#endif // SOUND_MANAGER_H