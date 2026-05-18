#ifndef RENDERER_H
#define RENDERER_H

#include <windows.h>
#include "player.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define FOV 1.04719755f // 60 degrés en radians

// Initialisation des textures (À appeler une seule fois)
void init_textures();
void init_weapon_asset();
void cleanup_weapon_asset();

void draw_3d_view(HWND hwnd, HDC hdc, Player p);

#endif