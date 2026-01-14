/****************************************
 * audio.cpp
 *
 * 概要:
 *   XAudio2 を用いたサウンド管理モジュール
 *   - 初期化 / 解放処理
 *   - WAV/MP3ファイルの読み込み
 *   - サウンド再生 / 解放
 *
 * 主な機能:
 *   InitAudio()       : オーディオシステムの初期化
 *   UninitAudio()     : オーディオシステムの解放
 *   LoadAudio()       : WAV/MP3ファイル読み込み
 *   UnloadAudio()     : サウンド解放
 *   PlayAudio()       : サウンド再生
 *
 * @update 2026/01/13 - ファイル未存在時のクラッシュ防止
 * @update 2026/01/13 - MP3対応（Media Foundation使用）
 ****************************************/

#include "audio.h"
#include <assert.h>
#include <xaudio2.h>
#include <string>
#include <vector>
#include <algorithm>

#include <filesystem>
 // Media Foundation（MP3デコード用）
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

//--------------------------------------
// グローバル変数群
//--------------------------------------
static IXAudio2* g_Xaudio{};
static IXAudio2MasteringVoice* g_MasteringVoice{};
static bool g_MFInitialized = false;

//======================================
// 初期化・終了処理
//======================================

void InitAudio()
{
    // XAudio2オブジェクト生成
    XAudio2Create(&g_Xaudio, 0);

    // マスタリングボイス生成
    g_Xaudio->CreateMasteringVoice(&g_MasteringVoice);

    // Media Foundation初期化（MP3用）
    HRESULT hr = MFStartup(MF_VERSION);
    g_MFInitialized = SUCCEEDED(hr);
}

void UninitAudio()
{
    // Media Foundation終了
    if (g_MFInitialized)
    {
        MFShutdown();
        g_MFInitialized = false;
    }

    g_MasteringVoice->DestroyVoice();
    g_Xaudio->Release();
}

//======================================
// サウンドデータ構造体定義
//======================================

struct AUDIO
{
    IXAudio2SourceVoice* SourceVoice{};
    BYTE* SoundData{};
    int Length{};
    int PlayLength{};
};

#define AUDIO_MAX 100
static AUDIO g_Audio[AUDIO_MAX]{};

//======================================
// ヘルパー関数
//======================================

/**
 * @brief ファイル拡張子を取得（小文字）
 */
static std::string GetFileExtension(const char* filename)
{
    std::string path(filename);
    size_t pos = path.rfind('.');
    if (pos == std::string::npos)
        return "";

    std::string ext = path.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

/**
 * @brief マルチバイト文字列をワイド文字列に変換
 */
static std::wstring ToWideString(const char* str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, nullptr, 0);
    std::wstring wstr(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str, -1, &wstr[0], len);
    return wstr;
}

//======================================
// WAVファイル読み込み
//======================================

static int LoadWav(const char* FileName, int index)
{
    WAVEFORMATEX wfx = { 0 };

    HMMIO hmmio = nullptr;
    MMIOINFO mmioinfo = { 0 };
    MMCKINFO riffchunkinfo = { 0 };
    MMCKINFO datachunkinfo = { 0 };
    MMCKINFO mmckinfo = { 0 };
    UINT32 buflen;
    LONG readlen;

    // ファイルオープン
    hmmio = mmioOpen((LPSTR)FileName, &mmioinfo, MMIO_READ);

    if (hmmio == nullptr)
    {
#if defined(DEBUG) || defined(_DEBUG)
        OutputDebugStringA("Warning: Audio file not found: ");
        OutputDebugStringA(FileName);
        OutputDebugStringA("\n");
#endif
        return -1;
    }

    // "RIFF" → "WAVE" チャンク検索
    riffchunkinfo.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    mmioDescend(hmmio, &riffchunkinfo, nullptr, MMIO_FINDRIFF);

    // "fmt " チャンク読み込み
    mmckinfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
    mmioDescend(hmmio, &mmckinfo, &riffchunkinfo, MMIO_FINDCHUNK);

    if (mmckinfo.cksize >= sizeof(WAVEFORMATEX))
    {
        mmioRead(hmmio, (HPSTR)&wfx, sizeof(wfx));
    }
    else
    {
        PCMWAVEFORMAT pcmwf = { 0 };
        mmioRead(hmmio, (HPSTR)&pcmwf, sizeof(pcmwf));
        memset(&wfx, 0x00, sizeof(wfx));
        memcpy(&wfx, &pcmwf, sizeof(pcmwf));
        wfx.cbSize = 0;
    }
    mmioAscend(hmmio, &mmckinfo, 0);

    // "data" チャンク読み込み
    datachunkinfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
    mmioDescend(hmmio, &datachunkinfo, &riffchunkinfo, MMIO_FINDCHUNK);

    // PCMデータ確保・読み込み
    buflen = datachunkinfo.cksize;
    g_Audio[index].SoundData = new unsigned char[buflen];
    readlen = mmioRead(hmmio, (HPSTR)g_Audio[index].SoundData, buflen);

    g_Audio[index].Length = readlen;
    g_Audio[index].PlayLength = readlen / wfx.nBlockAlign;

    mmioClose(hmmio, 0);

    // SourceVoice作成
    g_Xaudio->CreateSourceVoice(&g_Audio[index].SourceVoice, &wfx);

    if (g_Audio[index].SourceVoice == nullptr)
    {
        delete[] g_Audio[index].SoundData;
        g_Audio[index].SoundData = nullptr;
        return -1;
    }

    return index;
}

//======================================
// MP3ファイル読み込み（Media Foundation）
//======================================

static int LoadMp3(const char* FileName, int index)
{
    if (!g_MFInitialized)
    {
#if defined(DEBUG) || defined(_DEBUG)
        OutputDebugStringA("Warning: Media Foundation not initialized\n");
#endif
        return -1;
    }

    std::wstring wpath = ToWideString(FileName);

    IMFSourceReader* pReader = nullptr;
    HRESULT hr;

    // SourceReader作成
    hr = MFCreateSourceReaderFromURL(wpath.c_str(), nullptr, &pReader);
    if (FAILED(hr))
    {
#if defined(DEBUG) || defined(_DEBUG)
        OutputDebugStringA("Warning: Audio file not found: ");
        OutputDebugStringA(FileName);
        OutputDebugStringA("\n");
#endif
        return -1;
    }

    // 出力フォーマットをPCMに設定
    IMFMediaType* pOutputType = nullptr;
    MFCreateMediaType(&pOutputType);
    pOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    pOutputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);

    hr = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pOutputType);
    pOutputType->Release();

    if (FAILED(hr))
    {
        pReader->Release();
        return -1;
    }

    // 実際の出力フォーマットを取得
    IMFMediaType* pActualType = nullptr;
    hr = pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pActualType);
    if (FAILED(hr))
    {
        pReader->Release();
        return -1;
    }

    // WAVEFORMATEX取得
    WAVEFORMATEX* pWfx = nullptr;
    UINT32 wfxSize = 0;
    hr = MFCreateWaveFormatExFromMFMediaType(pActualType, &pWfx, &wfxSize);
    pActualType->Release();

    if (FAILED(hr))
    {
        pReader->Release();
        return -1;
    }

    // WAVEFORMATEXをコピー
    WAVEFORMATEX wfx;
    memcpy(&wfx, pWfx, sizeof(WAVEFORMATEX));
    CoTaskMemFree(pWfx);

    // 全サンプルを読み込む
    std::vector<BYTE> audioData;

    while (true)
    {
        IMFSample* pSample = nullptr;
        DWORD dwFlags = 0;

        hr = pReader->ReadSample(
            MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            0,
            nullptr,
            &dwFlags,
            nullptr,
            &pSample
        );

        if (FAILED(hr) || (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM))
        {
            if (pSample) pSample->Release();
            break;
        }

        if (pSample)
        {
            IMFMediaBuffer* pBuffer = nullptr;
            hr = pSample->ConvertToContiguousBuffer(&pBuffer);

            if (SUCCEEDED(hr))
            {
                BYTE* pAudioData = nullptr;
                DWORD cbBuffer = 0;

                hr = pBuffer->Lock(&pAudioData, nullptr, &cbBuffer);
                if (SUCCEEDED(hr))
                {
                    size_t oldSize = audioData.size();
                    audioData.resize(oldSize + cbBuffer);
                    memcpy(&audioData[oldSize], pAudioData, cbBuffer);
                    pBuffer->Unlock();
                }
                pBuffer->Release();
            }
            pSample->Release();
        }
    }

    pReader->Release();

    if (audioData.empty())
    {
        return -1;
    }

    // オーディオデータをコピー
    g_Audio[index].SoundData = new BYTE[audioData.size()];
    memcpy(g_Audio[index].SoundData, audioData.data(), audioData.size());
    g_Audio[index].Length = static_cast<int>(audioData.size());
    g_Audio[index].PlayLength = g_Audio[index].Length / wfx.nBlockAlign;

    // SourceVoice作成
    hr = g_Xaudio->CreateSourceVoice(&g_Audio[index].SourceVoice, &wfx);

    if (FAILED(hr) || g_Audio[index].SourceVoice == nullptr)
    {
        delete[] g_Audio[index].SoundData;
        g_Audio[index].SoundData = nullptr;
        return -1;
    }

    return index;
}

//======================================
// 統合ロード関数
//======================================

int LoadAudio(const char* FileName)
{
    int index = -1;

    // 空きスロット検索
    for (int i = 0; i < AUDIO_MAX; i++)
    {
        if (g_Audio[i].SourceVoice == nullptr)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
        return -1;

    // 拡張子で処理を分岐
    std::string ext = GetFileExtension(FileName);

    if (ext == ".wav")
    {
        return LoadWav(FileName, index);
    }
    else if (ext == ".mp3")
    {
        return LoadMp3(FileName, index);
    }
    else
    {
        // 不明な拡張子はWAVとして試す
#if defined(DEBUG) || defined(_DEBUG)
        OutputDebugStringA("Warning: Unknown audio format, trying as WAV: ");
        OutputDebugStringA(FileName);
        OutputDebugStringA("\n");
#endif
        return LoadWav(FileName, index);
    }
}

//======================================
// サウンド解放
//======================================

void UnloadAudio(int Index)
{
    if (Index < 0 || Index >= AUDIO_MAX)
        return;

    if (g_Audio[Index].SourceVoice == nullptr)
        return;

    g_Audio[Index].SourceVoice->Stop();
    g_Audio[Index].SourceVoice->DestroyVoice();
    g_Audio[Index].SourceVoice = nullptr;

    if (g_Audio[Index].SoundData)
    {
        delete[] g_Audio[Index].SoundData;
        g_Audio[Index].SoundData = nullptr;
    }

    g_Audio[Index].Length = 0;
    g_Audio[Index].PlayLength = 0;
}

//======================================
// サウンド停止
//======================================

void StopAudio(int Index)
{
    if (Index < 0 || Index >= AUDIO_MAX)
        return;

    if (g_Audio[Index].SourceVoice == nullptr)
        return;

    g_Audio[Index].SourceVoice->Stop();
    g_Audio[Index].SourceVoice->FlushSourceBuffers();
}

void StopAllAudio()
{
    for (int i = 0; i < AUDIO_MAX; i++)
    {
        if (g_Audio[i].SourceVoice != nullptr)
        {
            g_Audio[i].SourceVoice->Stop();
            g_Audio[i].SourceVoice->FlushSourceBuffers();
        }
    }
}

//======================================
// サウンド再生
//======================================

void PlayAudio(int Index, bool Loop)
{
    if (Index < 0 || Index >= AUDIO_MAX)
        return;

    if (g_Audio[Index].SourceVoice == nullptr)
        return;

    // 再生停止・バッファクリア
    g_Audio[Index].SourceVoice->Stop();
    g_Audio[Index].SourceVoice->FlushSourceBuffers();

    // バッファ設定
    XAUDIO2_BUFFER bufinfo = {};
    bufinfo.AudioBytes = g_Audio[Index].Length;
    bufinfo.pAudioData = g_Audio[Index].SoundData;
    bufinfo.PlayBegin = 0;
    bufinfo.PlayLength = g_Audio[Index].PlayLength;

    if (Loop)
    {
        bufinfo.LoopBegin = 0;
        bufinfo.LoopLength = g_Audio[Index].PlayLength;
        bufinfo.LoopCount = XAUDIO2_LOOP_INFINITE;
    }

    g_Audio[Index].SourceVoice->SubmitSourceBuffer(&bufinfo, nullptr);
    g_Audio[Index].SourceVoice->Start();
}

//======================================
// ボリューム制御
//======================================

void SetAudioVolume(int Index, float volume)
{
    if (Index < 0 || Index >= AUDIO_MAX)
        return;

    if (g_Audio[Index].SourceVoice == nullptr)
        return;

    // ボリュームをクランプ（0.0〜2.0、1.0が標準）
    volume = std::fmaxf(0.0f, std::fminf(2.0f, volume));
    g_Audio[Index].SourceVoice->SetVolume(volume);
}

void SetMasterVolume(float volume)
{
    if (g_MasteringVoice == nullptr)
        return;

    // ボリュームをクランプ
    volume = std::fmaxf(0.0f, std::fminf(2.0f, volume));
    g_MasteringVoice->SetVolume(volume);
}