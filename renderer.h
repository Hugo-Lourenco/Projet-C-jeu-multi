#ifndef RENDERER_H
#define RENDERER_H

#include <stdbool.h>
#include <windows.h>
#include "player.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define FOV 1.04719755f // 60 degrés en radians

#ifdef __cplusplus
extern "C"
{
#endif

    // Initialisation des textures (À appeler une seule fois)
    void init_textures();
    bool init_weapon_asset();
    void cleanup_weapon_asset();
    void draw_weapon_asset(HDC hdc, int x, int y, int width, int height, int frame, int recoil);

    void draw_3d_view(HWND hwnd, HDC hdc, Player p);
    void draw_tutorial(HDC hdc);

#ifdef __cplusplus
}
#endif

#endif