/****************************************
 * @file input_manager.h
 * @brief 入力管理（マウス状態の一元管理）
 * @author Natsume Shidara
 * @date 2026/01/07
 ****************************************/

#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "mouse.h"

 //======================================
 // 初期化・終了
 //======================================
void InputManager_Initialize();
void InputManager_Finalize();

//======================================
// 更新（フレームの最初に1回だけ呼ぶ）
//======================================
void InputManager_Update();

//======================================
// マウス状態取得（何度呼んでも同じ値を返す）
//======================================
const Mouse_State& InputManager_GetMouseState();

#endif // INPUT_MANAGER_H