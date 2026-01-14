/**
 * @file fade.h
 * @brief フェードイン・アウト制御
 * @author Natsume Shidara
 * @date 2025/07/10
 */

#ifndef FADE_H
#define FADE_H
#include "color.h"

void Fade_Initialize(void);
void Fade_Finalize(void);
void Fade_Update(double elapsed_time);
void Fade_Draw(void);

void Fade_Start(double duration, bool isFadeOut, Color::COLOR color = Color::BLACK);

enum class FADE_STATE
{
    NONE,
    FADE_IN,
    FADE_OUT,
    FINISHED_IN,
    FINISHED_OUT
};

FADE_STATE Fade_GetState(void);

#endif
