/**
 * @file scene.h
 * @brief ÉVÅ[Éìä«óù
 * @author Natsume Shidara
 * @date 2025/07/10
 * @update 2026/01/13 - SettingsâÊñ í«â¡
 */

#ifndef SCENE_H
#define SCENE_H

enum class Scene
{
    TITLE,
    GAME,
    RESULT,
    SETTINGS,  // ê›íËâÊñ 
};

void Scene_Initialize();
void Scene_Finalize();
void Scene_Update(double elapsed_time);
void Scene_Draw();
void Scene_Refresh();
void Scene_Change(Scene scene);

#endif // SCENE_H