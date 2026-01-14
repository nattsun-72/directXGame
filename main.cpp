#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "game_window.h"
#include "direct3d.h"
#include "sprite.h"
#include "texture.h"
#include "sprite_anim.h"
#include "debug_text.h"
#include <sstream>

#include "sound_manager.h"  // ← audio.h の代わりに使用
#include "game_settings.h"  // ゲーム設定管理
#include "fade.h"
#include "system_timer.h"
#include "key_logger.h"
#include "pad_logger.h"
#include "mouse.h"
#include "scene.h"

#include "cube.h"
#include "grid.h"
#include "meshfield.h"
#include "light.h"

#include "sampler.h"

#include "shader.h"
#include "shader3d.h"
#include "shader_billboard.h"
#include "shader3d_unlit.h"

/*----------------------------------------------------------------------------------
    メイン
----------------------------------------------------------------------------------*/
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR,
    _In_ int nCmdShow)
{
    (void)CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HWND hWnd = GameWindow_Create(hInstance);

    SystemTimer_Initialize();
    KeyLogger_Initialize();
    Mouse_Initialize(hWnd);
    PadLogger_Initialize();

    // サウンドマネージャー初期化（InitAudio()の代わり）
    SoundManager_Initialize();

    // ゲーム設定初期化
    GameSettings_Initialize();

    Direct3D_Initialize(hWnd);
    Sampler_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Shader3D_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Shader_Billboard_Initialize();
    Shader3D_Unlit_Initialize();

    Sprite_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Texture_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    SpriteAnim_Initialize();
    Fade_Initialize();

    Grid_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    MeshField_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Cube_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Light_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());

    // デバッグテキスト
    hal::DebugText debugText(Direct3D_GetDevice(), Direct3D_GetContext(),
        L"assets/consolab_ascii_512.png",
        Direct3D_GetBackBufferWidth(), Direct3D_GetBackBufferHeight(),
        0.0f, 0.0f, 0, 0, 0.0f, 16.0f);

    Mouse_SetVisible(true);
    Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);

    Scene_Initialize();

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // fps・実行フレーム計測用
    double exec_last_time = SystemTimer_GetTime();
    double fps_last_time = exec_last_time;
    double current_time = 0.0;
    ULONG frame_count = 0;
    double fps = 0;
    double elapsed_time = 0.0;

    MSG msg;

    do
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            current_time = SystemTimer_GetTime();
            double fps_elapsed_time = current_time - fps_last_time;

            if (fps_elapsed_time >= 1.0)
            {
                fps = frame_count / fps_elapsed_time;
                fps_last_time = current_time;
                frame_count = 0;
            }

            elapsed_time += SystemTimer_GetElapsedTime();

            if (true)
            {
                KeyLogger_Update();
                PadLogger_Update();
                Scene_Update(elapsed_time);
                SpriteAnim_Update(elapsed_time);
                Fade_Update(elapsed_time);

                //----------------------------------------------------------
                // 描画処理
                //----------------------------------------------------------
                // Scene_Draw 内で「オフスクリーン」→「メイン描画」→「UI」の全てを完結させる
                Scene_Draw();

                // フェード・デバッグ表示（最前面）
                Direct3D_SetBlendState(BlendMode::Alpha);
                Fade_Draw();

#if defined(DEBUG) || defined(_DEBUG)
                std::stringstream ss;
                ss << "fps:" << fps << std::endl;
                debugText.SetText(ss.str().c_str());
                debugText.Draw();
                debugText.Clear();
#endif

                Direct3D_Present();
                Scene_Refresh();

                frame_count++;
                elapsed_time = 0.0;
            }
        }
    } while (msg.message != WM_QUIT);

    Light_Finalize();
    Cube_Finalize();
    MeshField_Finalize();
    Grid_Finalize();

    Scene_Finalize();

    // ゲーム設定終了
    GameSettings_Finalize();

    // サウンドマネージャー終了
    SoundManager_Finalize();

    Fade_Finalize();
    SpriteAnim_Finalize();
    Texture_Finalize();
    Sprite_Finalize();
    Sampler_Finalize();
    Direct3D_Finalize();
    Mouse_Finalize();

    Shader_Finalize();
    Shader3D_Finalize();
    Shader_Billboard_Finalize();
    Shader3D_Unlit_Finalize();

    return static_cast<int>(msg.wParam);
}