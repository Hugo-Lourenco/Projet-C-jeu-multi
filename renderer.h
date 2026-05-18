#ifndef RENDERER_H
#define RENDERER_H

#include <windows.h>
#include "player.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define FOV 1.0472f // 60 degrés en radians

void draw_3d_view(HWND hwnd, HDC hdc, Player p);

#endif