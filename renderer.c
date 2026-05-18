#include "renderer.h"
#include "map.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern Player Packets_players[5];

uint32_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
uint32_t tex_wall[64 * 64];
uint32_t tex_enemy[64 * 64];

uint32_t get_color(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 16) | (g << 8) | b;
}

void init_textures() {
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            if (y % 32 == 0 || y % 32 == 31 || (y < 32 && x % 32 == 0) || (y >= 32 && (x + 16) % 32 == 0)) {
                tex_wall[y * 64 + x] = get_color(80, 80, 80);
            } else { 
                int grain = (x * y) % 20;
                tex_wall[y * 64 + x] = get_color(180 - grain, 50 - grain/2, 50 - grain/2);
            }
        }
    }

    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            tex_enemy[y * 64 + x] = 0; 
            float cx = x - 32; float cy = y - 32;
            if (y > 10 && y < 25 && cx*cx + (y-17)*(y-17) < 60) tex_enemy[y * 64 + x] = get_color(220, 180, 150);
            if (y > 14 && y < 19 && x > 25 && x < 45) tex_enemy[y * 64 + x] = get_color(255, 0, 0);
            if (y >= 25 && y < 55 && x > 15 && x < 49) tex_enemy[y * 64 + x] = get_color(40, 40, 40);
            if (y >= 55 && ( (x > 15 && x < 28) || (x > 36 && x < 49) )) tex_enemy[y * 64 + x] = get_color(30, 30, 30);
        }
    }
}

void draw_3d_view(HWND hwnd, HDC hdc, Player p) {
    float z_buffer[SCREEN_WIDTH];

    for (int y = 0; y < SCREEN_HEIGHT / 2; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
            framebuffer[y * SCREEN_WIDTH + x] = get_color(50, 60, 70); 

    for (int y = SCREEN_HEIGHT / 2; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
            framebuffer[y * SCREEN_WIDTH + x] = get_color(30, 30, 30); 

    for (int x = 0; x < SCREEN_WIDTH; x++) {
        float ray_angle = (p.angle - FOV / 2.0f) + ((float)x / (float)SCREEN_WIDTH) * FOV;
        float distance_to_wall = 0.0f;
        bool hit_wall = false;
        
        float eye_x = cosf(ray_angle);
        float eye_y = sinf(ray_angle);

        while (!hit_wall && distance_to_wall < 16.0f) {
            distance_to_wall += 0.02f;
            int check_x = (int)(p.x + eye_x * distance_to_wall);
            int check_y = (int)(p.y + eye_y * distance_to_wall);

            if (check_x < 0 || check_x >= MAP_WIDTH || check_y < 0 || check_y >= MAP_HEIGHT) {
                hit_wall = true; distance_to_wall = 16.0f;
            } else if (game_map[check_y][check_x] == 1) {
                hit_wall = true;
            }
        }

        float corrected_dist = distance_to_wall * cosf(ray_angle - p.angle);
        if (corrected_dist < 0.1f) corrected_dist = 0.1f;
        z_buffer[x] = corrected_dist;

        float exact_hit_x = p.x + eye_x * distance_to_wall;
        float exact_hit_y = p.y + eye_y * distance_to_wall;
        float frac_x = exact_hit_x - floorf(exact_hit_x);
        float frac_y = exact_hit_y - floorf(exact_hit_y);

        float diff_x = frac_x < 0.5f ? frac_x : 1.0f - frac_x;
        float diff_y = frac_y < 0.5f ? frac_y : 1.0f - frac_y;
        
        int tex_x;
        bool is_dark = false; 
        if (diff_x < diff_y) {
            tex_x = (int)(frac_y * 64.0f);
            is_dark = true;
        } else {
            tex_x = (int)(frac_x * 64.0f);
        }
        if (tex_x < 0) tex_x = 0; if (tex_x > 63) tex_x = 63;

        int wall_height = (int)((float)SCREEN_HEIGHT / (corrected_dist * 1.5f));
        int wall_ceiling = (SCREEN_HEIGHT / 2) - (wall_height / 2);
        int wall_floor = (SCREEN_HEIGHT / 2) + (wall_height / 2);

        for (int y = wall_ceiling; y < wall_floor; y++) {
            if (y < 0 || y >= SCREEN_HEIGHT) continue;
            int d = y - wall_ceiling;
            int tex_y = (d * 64) / wall_height;
            if (tex_y < 0) tex_y = 0; if (tex_y > 63) tex_y = 63;

            uint32_t color = tex_wall[tex_y * 64 + tex_x];
            if (is_dark) color = (color & 0xFEFEFE) >> 1; 
            framebuffer[y * SCREEN_WIDTH + x] = color;
        }
    }

    for (int i = 1; i <= 4; i++) {
        if (i == p.id || Packets_players[i].x == 0 || !Packets_players[i].is_alive) continue;

        float sprite_x = Packets_players[i].x - p.x;
        float sprite_y = Packets_players[i].y - p.y;
        float sprite_angle = atan2f(sprite_y, sprite_x) - p.angle;

        while (sprite_angle < -3.14159f) sprite_angle += 2.0f * 3.14159f;
        while (sprite_angle > 3.14159f)  sprite_angle -= 2.0f * 3.14159f;

        float sprite_dist = sqrtf(sprite_x * sprite_x + sprite_y * sprite_y);
        if (sprite_dist < 0.1f) sprite_dist = 0.1f;

        if (fabs(sprite_angle) < FOV) {
            int sprite_screen_x = (int)((SCREEN_WIDTH / 2) + (sprite_angle / FOV) * SCREEN_WIDTH);
            int sprite_size = (int)(SCREEN_HEIGHT / (sprite_dist * 1.5f));
            int sprite_ceil = SCREEN_HEIGHT / 2 - sprite_size / 2;
            int sprite_floor = SCREEN_HEIGHT / 2 + sprite_size / 2;
            int start_x = sprite_screen_x - sprite_size / 2;
            int end_x = sprite_screen_x + sprite_size / 2;

            for (int x = start_x; x < end_x; x++) {
                if (x < 0 || x >= SCREEN_WIDTH || sprite_dist >= z_buffer[x]) continue;
                int tex_x = ((x - start_x) * 64) / sprite_size;
                if (tex_x < 0) tex_x = 0; if (tex_x > 63) tex_x = 63;

                for (int y = sprite_ceil; y < sprite_floor; y++) {
                    if (y < 0 || y >= SCREEN_HEIGHT) continue;
                    int tex_y = ((y - sprite_ceil) * 64) / sprite_size;
                    if (tex_y < 0) tex_y = 0; if (tex_y > 63) tex_y = 63;

                    uint32_t color = tex_enemy[tex_y * 64 + tex_x];
                    if (color != 0) framebuffer[y * SCREEN_WIDTH + x] = color;
                }
            }
        }
    }

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = SCREEN_WIDTH;
    bmi.bmiHeader.biHeight = -SCREEN_HEIGHT; 
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBitsToDevice(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, SCREEN_HEIGHT, framebuffer, &bmi, DIB_RGB_COLORS);

    // =========================================================
    // 5. NOUVELLE INTERFACE GDI (SANS LA CROIX MÉDICALE)
    // =========================================================

    // A. Le Viseur 
    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2;
    HPEN crossPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0)); 
    HPEN oldPen = (HPEN)SelectObject(hdc, crossPen);
    MoveToEx(hdc, center_x - 12, center_y, NULL); LineTo(hdc, center_x - 4, center_y);
    MoveToEx(hdc, center_x + 4, center_y, NULL); LineTo(hdc, center_x + 12, center_y);
    MoveToEx(hdc, center_x, center_y - 12, NULL); LineTo(hdc, center_x, center_y - 4);
    MoveToEx(hdc, center_x, center_y + 4, NULL); LineTo(hdc, center_x, center_y + 12);
    SelectObject(hdc, oldPen); DeleteObject(crossPen);

    // B. Paramètres de la barre de vie
    int bar_x = 20; // Recalée sur la gauche
    int bar_y = SCREEN_HEIGHT - 45;
    int max_width = 200;
    int bar_height = 20;

    // C. Contour sombre de la barre
    HBRUSH borderBrush = CreateSolidBrush(RGB(15, 15, 15));
    RECT borderRect = { bar_x - 2, bar_y - 2, bar_x + max_width + 2, bar_y + bar_height + 2 };
    FillRect(hdc, &borderRect, borderBrush);
    DeleteObject(borderBrush);

    // D. Fond de la barre
    HBRUSH bgBrush = CreateSolidBrush(RGB(60, 20, 20));
    RECT bgRect = { bar_x, bar_y, bar_x + max_width, bar_y + bar_height };
    FillRect(hdc, &bgRect, bgBrush);
    DeleteObject(bgBrush);

    // E. Remplissage coloré dynamique
    int current_width = (int)(((float)p.hp / 100.0f) * max_width);
    if (current_width < 0) current_width = 0;
    if (current_width > max_width) current_width = max_width;

    HBRUSH hpBrush;
    if (p.hp > 50) hpBrush = CreateSolidBrush(RGB(46, 204, 113));     
    else if (p.hp > 25) hpBrush = CreateSolidBrush(RGB(241, 196, 15)); 
    else hpBrush = CreateSolidBrush(RGB(231, 76, 60));                 
    
    RECT hpRect = { bar_x, bar_y, bar_x + current_width, bar_y + bar_height };
    FillRect(hdc, &hpRect, hpBrush);
    DeleteObject(hpBrush);

    // F. Effet "Armure segmentée" 
    HBRUSH segmentBrush = CreateSolidBrush(RGB(15, 15, 15));
    for(int i = 1; i < 10; i++) {
        int seg_x = bar_x + (i * (max_width / 10));
        RECT segRect = {seg_x, bar_y, seg_x + 2, bar_y + bar_height};
        FillRect(hdc, &segRect, segmentBrush);
    }
    DeleteObject(segmentBrush);

    // G. Texte des PV
    char hp_text[32];
    sprintf(hp_text, "%d / 100", p.hp);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, bar_x + max_width + 15, bar_y + 2, hp_text, strlen(hp_text));
}