[33mcommit f0ad6352833f31cfbd30b960a749868f966a3875[m[33m ([m[1;36mHEAD[m[33m -> [m[1;32mmaster[m[33m, [m[1;31morigin/master[m[33m)[m
Author: N-72 <bcnattsun@gmail.com>
Date:   Mon Feb 9 10:50:08 2026 +0900

    particle_second

 assets/shader/shader_pixel_billboard.cso   | Bin [31m14912[m -> [32m14872[m bytes
 billboard.cpp                              |  30 [32m++[m[31m----[m
 direct3d/direct3d.cpp                      |  47 [32m+++++++[m[31m--[m
 game/scene/game.cpp                        | 163 [32m++++++++++++++[m[31m---------------[m
 particle.h                                 |   6 [32m+[m[31m-[m
 particle_test.cpp                          |  74 [32m++++++++++[m[31m---[m
 particle_test.h                            |  15 [32m+[m[31m--[m
 shader_billboard.cpp                       |   2 [32m+[m[31m-[m
 shader_pixel_billboard.hlsl                |  13 [32m+[m[31m--[m
 stage.cpp                                  |   2 [32m+[m[31m-[m
 x64/Debug/DirectX3D.ilk                    | Bin [31m25028200[m -> [32m26162424[m bytes
 x64/Debug/DirectX3D.log                    |   2 [32m+[m[31m-[m
 x64/Debug/DirectX3D.tlog/CL.command.1.tlog | Bin [31m95260[m -> [32m95260[m bytes
 x64/Debug/DirectX3D.tlog/CL.read.1.tlog    | Bin [31m3114796[m -> [32m3115538[m bytes
 x64/Debug/DirectX3D.tlog/CL.write.1.tlog   | Bin [31m33678[m -> [32m35716[m bytes
 x64/Debug/DirectX3D.tlog/fxc.read.1.tlog   | Bin [31m3734[m -> [32m3734[m bytes
 x64/Debug/DirectX3D.tlog/fxc.write.1.tlog  | Bin [31m3386[m -> [32m3386[m bytes
 x64/Debug/billboard.obj                    | Bin [31m958273[m -> [32m958153[m bytes
 x64/Debug/direct3d.obj                     | Bin [31m777617[m -> [32m777617[m bytes
 x64/Debug/game.obj                         | Bin [31m425454[m -> [32m415866[m bytes
 x64/Debug/particle.obj                     | Bin [31m65586[m -> [32m65586[m bytes
 x64/Debug/particle_test.obj                | Bin [31m369094[m -> [32m387464[m bytes
 x64/Debug/scene.obj                        | Bin [31m288828[m -> [32m288828[m bytes
 x64/Debug/shader_billboard.obj             | Bin [31m1365947[m -> [32m1365997[m bytes
 x64/Debug/slice_task_manager.obj           | Bin [31m962912[m -> [32m962912[m bytes
 x64/Debug/stage.obj                        | Bin [31m373526[m -> [32m373429[m bytes
 x64/Debug/vc143.idb                        | Bin [31m1174528[m -> [32m1174528[m bytes
 x64/Debug/vc143.pdb                        | Bin [31m2994176[m -> [32m2994176[m bytes
 28 files changed, 200 insertions(+), 154 deletions(-)

[33mcommit 9f612eefed2c50d3f71c00cc5f80ff3fe5565fb9[m
Author: N-72 <bcnattsun@gmail.com>
Date:   Fri Feb 6 14:40:58 2026 +0900

    effect_first_commit

 DirectX3D.vcxproj                              |   4 [32m+[m
 DirectX3D.vcxproj.filters                      |  12 [32m+++[m
 assets/effect000.jpg                           | Bin [31m0[m -> [32m2375[m bytes
 billboard.cpp                                  |   3 [32m+[m[31m-[m
 game/scene/game.cpp                            |  35 [32m+++++[m[31m-[m
 particle.cpp                                   |  38 [32m+++++++[m
 particle.h                                     | 143 [32m+++++++++++++++++++++++++[m
 particle_test.cpp                              |  52 [32m+++++++++[m
 particle_test.h                                |  56 [32m++++++++++[m
 x64/Debug/DirectX3D.ilk                        | Bin [31m25985776[m -> [32m25028200[m bytes
 x64/Debug/DirectX3D.log                        |   7 [32m+[m[31m-[m
 x64/Debug/DirectX3D.tlog/CL.command.1.tlog     | Bin [31m92660[m -> [32m95260[m bytes
 x64/Debug/DirectX3D.tlog/CL.read.1.tlog        | Bin [31m3053928[m -> [32m3114796[m bytes
 x64/Debug/DirectX3D.tlog/CL.write.1.tlog       | Bin [31m31660[m -> [32m33678[m bytes
 x64/Debug/DirectX3D.tlog/Cl.items.tlog         |   2 [32m+[m
 x64/Debug/DirectX3D.tlog/link.command.1.tlog   | Bin [31m12028[m -> [32m12352[m bytes
 x64/Debug/DirectX3D.tlog/link.read.1.tlog      | Bin [31m19872[m -> [32m20312[m bytes
 x64/Debug/DirectX3D.tlog/link.secondary.1.tlog |   2 [32m+[m[31m-[m
 x64/Debug/DirectX3D.tlog/link.write.1.tlog     | Bin [31m7688[m -> [32m7906[m bytes
 x64/Debug/billboard.obj                        | Bin [31m958723[m -> [32m958273[m bytes
 x64/Debug/game.obj                             | Bin [31m374790[m -> [32m425454[m bytes
 x64/Debug/particle.obj                         | Bin [31m0[m -> [32m65586[m bytes
 x64/Debug/particle_test.obj                    | Bin [31m0[m -> [32m369094[m bytes
 x64/Debug/vc143.idb                            | Bin [31m1174528[m -> [32m1174528[m bytes
 x64/Debug/vc143.pdb                            | Bin [31m2977792[m -> [32m2994176[m bytes
 25 files changed, 344 insertions(+), 10 deletions(-)

[33mcommit bb8f4631e2fe8a3b5d2411c1d1f739e2f04efec6[m
Author: N-72 <bcnattsun@gmail.com>
Date:   Wed Feb 4 10:08:22 2026 +0900

    Shader_And_BladeMotion

 assets/floor.png                           | Bin [31m260051[m -> [32m9173391[m bytes
 assets/shader/shader_pixel_3d.cso          | Bin [31m52316[m -> [32m50364[m bytes
 blade.cpp                                  | 350 [32m++++++++++++++++++++++++[m[31m-----[m
 blade.h                                    |   8 [32m+[m[31m-[m
 enemy_ground.cpp                           |   2 [32m+[m[31m-[m
 game/scene/game.cpp                        |   6 [32m+[m[31m-[m
 shaders/shader_pixel_3d.hlsl               |  74 [32m+++[m[31m---[m
 x64/Debug/DirectX3D.ilk                    | Bin [31m26322232[m -> [32m25985776[m bytes
 x64/Debug/DirectX3D.log                    |  12 [32m+[m[31m-[m
 x64/Debug/DirectX3D.tlog/CL.command.1.tlog | Bin [31m92660[m -> [32m92660[m bytes
 x64/Debug/DirectX3D.tlog/CL.read.1.tlog    | Bin [31m3049134[m -> [32m3053928[m bytes
 x64/Debug/DirectX3D.tlog/CL.write.1.tlog   | Bin [31m30478[m -> [32m31660[m bytes
 x64/Debug/DirectX3D.tlog/fxc.read.1.tlog   | Bin [31m3734[m -> [32m3734[m bytes
 x64/Debug/DirectX3D.tlog/fxc.write.1.tlog  | Bin [31m3386[m -> [32m3386[m bytes
 x64/Debug/blade.obj                        | Bin [31m466875[m -> [32m540782[m bytes
 x64/Debug/enemy_ground.obj                 | Bin [31m511628[m -> [32m511600[m bytes
 x64/Debug/game.obj                         | Bin [31m374790[m -> [32m374790[m bytes
 x64/Debug/player.obj                       | Bin [31m480835[m -> [32m480835[m bytes
 x64/Debug/player_camera.obj                | Bin [31m364143[m -> [32m364143[m bytes
 x64/Debug/vc143.idb                        | Bin [31m1174528[m -> [32m1174528[m bytes
 x64/Debug/vc143.pdb                        | Bin [31m2961408[m -> [32m2977792[m bytes
 21 files changed, 343 insertions(+), 109 deletions(-)

[33mcommit b36d85552f7a3a6045655e8dc96f11d1454225d6[m
Author: N-72 <bcnattsun@gmail.com>
Date:   Wed Jan 28 10:49:27 2026 +0900

    playerCameraShake

 player.cpp                                 |   1 [32m+[m
 player_camera.cpp                          |  89 [32m+++++++++++++++++++++++++[m[31m----[m
 player_camera.h                            |   8 [32m+++[m
 x64/Debug/DirectX3D.ilk                    | Bin [31m24581280[m -> [32m26322232[m bytes
 x64/Debug/DirectX3D.log                    |  25 [32m+++[m[31m-----[m
 x64/Debug/DirectX3D.tlog/CL.command.1.tlog | Bin [31m92660[m -> [32m92660[m bytes
 x64/Debug/DirectX3D.tlog/CL.read.1.tlog    | Bin [31m3049360[m -> [32m3049134[m bytes
 x64/Debug/DirectX3D.tlog/CL.write.1.tlog   | Bin [31m28044[m -> [32m30478[m bytes
 x64/Debug/DirectX3D.tlog/link.read.1.tlog  | Bin [31m20768[m -> [32m19872[m bytes
 x64/Debug/billboard.obj                    | Bin [31m958727[m -> [32m958723[m bytes
 x64/Debug/blade.obj                        | Bin [31m466875[m -> [32m466875[m bytes
 x64/Debug/game.obj                         | Bin [31m374790[m -> [32m374790[m bytes
 x64/Debug/map.obj                          | Bin [31m572499[m -> [32m572499[m bytes
 x64/Debug/player.obj                       | Bin [31m480755[m -> [32m480835[m bytes
 x64/Debug/player_camera.obj                | Bin [31m362298[m -> [32m364143[m bytes
 x64/Debug/ui.obj                           | Bin [31m267744[m -> [32m267744[m bytes
 x64/Debug/vc143.idb                        | Bin [31m1174528[m -> [32m1174528[m bytes
 x64/Debug/vc143.pdb                        | Bin [31m2961408[m -> [32m2961408[m bytes
 18 files changed, 94 insertions(+), 29 deletions(-)

[33mcommit 2d97fe381d9a4597b5491937c90076123f28bf22[m
Author: N-72 <bcnattsun@gmail.com>
Date:   Wed Jan 14 19:06:33 2026 +0900

    first init

 DirectX3D.filters                                  |   137 [32m+[m
 DirectX3D.vcxproj                                  |   491 [32m+[m
 DirectX3D.vcxproj.filters                          |   529 [32m+[m
 DirectX3D.vcxproj.user                             |     4 [32m+[m
 assets/.mayaSwatches/floor.png_hcm.swatch          |   Bin [31m0[m -> [32m12084[m bytes
 assets/.mayaSwatches/wall.png_hcm.swatch           |   Bin [31m0[m -> [32m5364[m bytes
 assets/Explosion.png                               |   Bin [31m0[m -> [32m848203[m bytes
 assets/box.png                                     |   Bin [31m0[m -> [32m1801342[m bytes
 assets/consolab_ascii_512.png                      |   Bin [31m0[m -> [32m18262[m bytes
 assets/fbx/.mayaSwatches/enemy.png_hcm.swatch      |   Bin [31m0[m -> [32m12588[m bytes
 assets/fbx/BBOXX.fbx                               |   Bin [31m0[m -> [32m27488[m bytes
 assets/fbx/BOX.fbx                                 |   Bin [31m0[m -> [32m14956[m bytes
 assets/fbx/LongSword/LongSword.fbx                 |   Bin [31m0[m -> [32m1253388[m bytes
 assets/fbx/LongSword/tex/DefaultMaterial_Alpha.jpg |   Bin [31m0[m -> [32m264044[m bytes
 .../LongSword/tex/DefaultMaterial_BaseColor.jpg    |   Bin [31m0[m -> [32m1256428[m bytes
 .../LongSword/tex/DefaultMaterial_Displacement.jpg |   Bin [31m0[m -> [32m436623[m bytes
 .../fbx/LongSword/tex/DefaultMaterial_Emission.jpg |   Bin [31m0[m -> [32m262771[m bytes
 .../fbx/LongSword/tex/DefaultMaterial_Metallic.jpg |   Bin [31m0[m -> [32m277745[m bytes
 .../fbx/LongSword/tex/DefaultMaterial_Normal.jpg   |   Bin [31m0[m -> [32m1528407[m bytes
 .../LongSword/tex/DefaultMaterial_Roughness.jpg    |   Bin [31m0[m -> [32m2612485[m bytes
 ...03\211\343\203\231\343\202\244\343\202\257.jpg" |   Bin [31m0[m -> [32m1218497[m bytes
 assets/fbx/Pallone/Ball 3DS.3ds                    |   Bin [31m0[m -> [32m521580[m bytes
 assets/fbx/Pallone/Ball BLEND.blend                |   Bin [31m0[m -> [32m2402836[m bytes
 assets/fbx/Pallone/Ball DAE.dae                    |   235 [32m+[m
 assets/fbx/Pallone/Ball OBJ.obj                    | 45639 [32m+++++++++++++++++++[m
 assets/fbx/Pallone/Ball.fbx                        |   Bin [31m0[m -> [32m648480[m bytes
 assets/fbx/banana.png                              |   Bin [31m0[m -> [32m27391[m bytes
 assets/fbx/enemy.png                               |   Bin [31m0[m -> [32m9993664[m bytes
 assets/fbx/enemy_sl.png                            |   Bin [31m0[m -> [32m56977[m bytes
 assets/fbx/moai.fbx                                |   Bin [31m0[m -> [32m45968[m bytes
 assets/fbx/option_booster/drone_danmen.png         |   Bin [31m0[m -> [32m8588852[m bytes
 .../option_booster.fbm/OP-B_bake.jpg               |   Bin [31m0[m -> [32m873311[m bytes
 assets/fbx/option_booster/option_booster.fbx       |   Bin [31m0[m -> [32m970636[m bytes
 assets/fbx/option_booster/option_booster1.fbx      |   Bin [31m0[m -> [32m118368[m bytes
 assets/fbx/option_booster/option_booster2.fbx      |   Bin [31m0[m -> [32m95504[m bytes
 assets/fbx/option_booster/tex/OP-B_bake.jpg        |   Bin [31m0[m -> [32m873311[m bytes
 assets/fbx/option_booster/tex/OP_B_Alpha.png       |   Bin [31m0[m -> [32m58788[m bytes
 assets/fbx/option_booster/tex/OP_B_BaseColor.png   |   Bin [31m0[m -> [32m4149519[m bytes
 .../fbx/option_booster/tex/OP_B_Displacement.png   |   Bin [31m0[m -> [32m183007[m bytes
 assets/fbx/option_booster/tex/OP_B_Emission.png    |   Bin [31m0[m -> [32m49073[m bytes
 assets/fbx/option_booster/tex/OP_B_Metallic.png    |   Bin [31m0[m -> [32m1059965[m bytes
 assets/fbx/option_booster/tex/OP_B_Normal.png      |   Bin [31m0[m -> [32m2944837[m bytes
 assets/fbx/option_booster/tex/OP_B_Roughness.png   |   Bin [31m0[m -> [32m7450904[m bytes
 assets/fbx/sky.fbx                                 |   Bin [31m0[m -> [32m22209980[m bytes
 assets/fbx/suika.fbx                               |   Bin [31m0[m -> [32m18172[m bytes
 assets/fbx/suika.png                               |   Bin [31m0[m -> [32m46721[m bytes
 assets/floor.png                                   |   Bin [31m0[m -> [32m260051[m bytes
 assets/runningman001.png                           |   Bin [31m0[m -> [32m135597[m bytes
 assets/shader/shader_pixel_2d.cso                  |   Bin [31m0[m -> [32m14564[m bytes
 assets/shader/shader_pixel_3d.cso                  |   Bin [31m0[m -> [32m52316[m bytes
 assets/shader/shader_pixel_3d_unlit.cso            |   Bin [31m0[m -> [32m16592[m bytes
 assets/shader/shader_pixel_billboard.cso           |   Bin [31m0[m -> [32m14912[m bytes
 assets/shader/shader_pixel_field.cso               |   Bin [31m0[m -> [32m22748[m bytes
 .../shader/shader_postprocess_directional_ps.cso   |   Bin [31m0[m -> [32m38504[m bytes
 .../shader/shader_postprocess_passthrough_ps.cso   |   Bin [31m0[m -> [32m14504[m bytes
 assets/shader/shader_postprocess_radial_ps.cso     |   Bin [31m0[m -> [32m43152[m bytes
 assets/shader/shader_postprocess_vs.cso            |   Bin [31m0[m -> [32m16752[m bytes
 assets/shader/shader_vartex_billboard.cso          |   Bin [31m0[m -> [32m17524[m bytes
 assets/shader/shader_vertex_2d.cso                 |   Bin [31m0[m -> [32m17196[m bytes
 assets/shader/shader_vertex_3d.cso                 |   Bin [31m0[m -> [32m22328[m bytes
 assets/shader/shader_vertex_3d_unlit.cso           |   Bin [31m0[m -> [32m19608[m bytes
 assets/shader/shader_vertex_billboard.cso          |   Bin [31m0[m -> [32m17780[m bytes
 assets/shader/shader_vertex_field.cso              |   Bin [31m0[m -> [32m21508[m bytes
 assets/shader/shadow_map.hlsl                      |    50 [32m+[m
 assets/sound/bgm/game.mp3                          |   Bin [31m0[m -> [32m4077540[m bytes
 assets/sound/bgm/result.mp3                        |   Bin [31m0[m -> [32m7218154[m bytes
 assets/sound/bgm/title.mp3                         |   Bin [31m0[m -> [32m1162179[m bytes
 assets/sound/se/decide.mp3                         |   Bin [31m0[m -> [32m21315[m bytes
 assets/sound/se/enemy_fire.mp3                     |   Bin [31m0[m -> [32m11783[m bytes
 assets/sound/se/jump.mp3                           |   Bin [31m0[m -> [32m12537[m bytes
 assets/sound/se/slash.mp3                          |   Bin [31m0[m -> [32m25076[m bytes
 assets/sound/se/slash_heavy.mp3                    |   Bin [31m0[m -> [32m25920[m bytes
 assets/sound/se/step.mp3                           |   Bin [31m0[m -> [32m5014[m bytes
 assets/tile.jpg                                    |   Bin [31m0[m -> [32m18308[m bytes
 assets/tile001.jpg                                 |   Bin [31m0[m -> [32m57779[m bytes
 assets/ui/8x8White.png                             |   Bin [31m0[m -> [32m2824[m bytes
 assets/ui/button.png                               |   Bin [31m0[m -> [32m370[m bytes
 assets/ui/button_hover.png                         |   Bin [31m0[m -> [32m368[m bytes
 assets/ui/combo_text.png                           |   Bin [31m0[m -> [32m7389[m bytes
 assets/ui/font_ascii.png                           |   Bin [31m0[m -> [32m24619[m bytes
 assets/ui/heart.png                                |   Bin [31m0[m -> [32m3165[m bytes
 assets/ui/menu_cursor.png                          |   Bin [31m0[m -> [32m738[m bytes
 assets/ui/numbers.png                              |   Bin [31m0[m -> [32m10730[m bytes
 assets/ui/numbers_14x17.png                        |   Bin [31m0[m -> [32m2569[m bytes
 assets/ui/numbers_32x40.png                        |   Bin [31m0[m -> [32m4074[m bytes
 assets/ui/press_enter.png                          |   Bin [31m0[m -> [32m1715[m bytes
 assets/ui/press_start.png                          |   Bin [31m0[m -> [32m4712[m bytes
 assets/ui/result_bg.png                            |   Bin [