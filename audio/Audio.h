/****************************************
 * audio.h
 *
 * 概要:
 *   XAudio2 を用いたサウンド管理モジュール
 *
 * @update 2026/01/13 - StopAudio追加
 * @update 2026/01/13 - ボリューム制御追加
 ****************************************/

#ifndef AUDIO_H
#define AUDIO_H

 /**
  * @brief オーディオシステム初期化
  */
void InitAudio();

/**
 * @brief オーディオシステム解放
 */
void UninitAudio();

/**
 * @brief WAV/MP3ファイル読み込み
 * @param FileName 読み込むファイル名
 * @return int 読み込みに使用したインデックス（失敗時は -1）
 */
int LoadAudio(const char* FileName);

/**
 * @brief サウンドデータ解放
 * @param Index 解放するオーディオのインデックス
 */
void UnloadAudio(int Index);

/**
 * @brief サウンドを再生
 * @param Index 再生するオーディオのインデックス
 * @param Loop ループ再生するかどうか（true=ループ）
 */
void PlayAudio(int Index, bool Loop = false);

/**
 * @brief サウンドを停止
 * @param Index 停止するオーディオのインデックス
 */
void StopAudio(int Index);

/**
 * @brief 全サウンドを停止
 */
void StopAllAudio();

/**
 * @brief 個別サウンドのボリューム設定
 * @param Index オーディオのインデックス
 * @param volume ボリューム（0.0〜1.0）
 */
void SetAudioVolume(int Index, float volume);

/**
 * @brief マスターボリューム設定
 * @param volume ボリューム（0.0〜1.0）
 */
void SetMasterVolume(float volume);

#endif // AUDIO_H