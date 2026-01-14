/**
 * @file game_window.h
 * @brief ウィンドウの初期化関連
 * @author Natsume Shidara
 * @date 2025/06/06
 */

#ifndef GAME_WINDOW_H
#define GAME_WINDOW_H

#include <SDKDDKVer.h>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

HWND GameWindow_Create(_In_ HINSTANCE hInstance);

#endif // GAME_WINDOW_H