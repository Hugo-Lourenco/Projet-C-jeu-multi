#include "renderer.h"
#include "map.h"
#include <math.h>

void draw_3d_view(HWND hwnd, HDC hdc, Player p) {
    // Variable statique pour retenir le moment exact où le jeu a commencé à dessiner
    static DWORD start_time = 0;
    if (start_time == 0) {
        start_time = GetTickCount(); // Enregistre le "top chrono" au premier lancement
    }

    // 1. Dessiner le ciel (moitié supérieure de l'écran en bleu/gris)
    HBRUSH skyBrush = CreateSolidBrush(RGB(50, 60, 70));
    RECT skyRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2 };
    FillRect(hdc, &skyRect, skyBrush);
    DeleteObject(skyBrush);

    // 2. Dessiner le sol (moitié inférieure en gris foncé)
    HBRUSH floorBrush = CreateSolidBrush(RGB(30, 30, 30));
    RECT floorRect = { 0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT };
    FillRect(hdc, &floorRect, floorBrush);
    DeleteObject(floorBrush);

    // 3. Lancer un rayon pour chaque colonne virtuelle de l'écran
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
                hit_wall = true;
                distance_to_wall = 16.0f;
            } else if (game_map[check_y][check_x] == 1) {
                hit_wall = true;
            }
        }

        float corrected_dist = distance_to_wall * cosf(ray_angle - p.angle);
        if (corrected_dist < 0.1f) corrected_dist = 0.1f;

        int wall_ceiling = (int)((float)(SCREEN_HEIGHT / 2.0f) - (float)SCREEN_HEIGHT / (corrected_dist * 1.5f));
        int wall_floor = SCREEN_HEIGHT - wall_ceiling;

        int color_val = (int)(255.0f / (1.0f + corrected_dist * corrected_dist * 0.1f));
        if (color_val > 255) color_val = 255;
        if (color_val < 0) color_val = 0;

        HBRUSH wallBrush = CreateSolidBrush(RGB(color_val, color_val, color_val));
        RECT wallColumn = { x, wall_ceiling, x + 1, wall_floor };
        FillRect(hdc, &wallColumn, wallBrush);
        DeleteObject(wallBrush);
    }

    // 4. DESSINER LE VISEUR (CROSSHAIR) AU CENTRE
    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2;
    int size = 10; 

    HPEN crossPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0)); 
    HPEN oldPen = (HPEN)SelectObject(hdc, crossPen);

    MoveToEx(hdc, center_x - size, center_y, NULL);
    LineTo(hdc, center_x + size, center_y);
    MoveToEx(hdc, center_x, center_y - size, NULL);
    LineTo(hdc, center_x, center_y + size);

    SelectObject(hdc, oldPen);
    DeleteObject(crossPen);

    // 5. ATH - TEXTE DE TUTORIEL (AFFICHÉ UNIQUEMENT PENDANT 10 SECONDES)
    // 10 secondes = 10 000 millisecondes
    if (GetTickCount() - start_time < 10000) {
        SetTextColor(hdc, RGB(0, 255, 0));
        SetBkMode(hdc, TRANSPARENT);

        TextOut(hdc, 15, 15,  "--- TUTORIEL DE JEU ---", 23);
        TextOut(hdc, 15, 40,  "Z : Avancer", 11);
        TextOut(hdc, 15, 60,  "S : Reculer", 11);
        TextOut(hdc, 15, 80,  "Q : Gauche", 17);
        TextOut(hdc, 15, 100, "D : Droite", 17);
        TextOut(hdc, 15, 125, "Souris : Tourner la camera", 26);
        TextOut(hdc, 15, 145, "Clic Gauche : Tirer", 19);
        TextOut(hdc, 15, 170, "Echap : Quitter le jeu", 22);
    }
}