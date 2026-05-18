#include "renderer.h"
#include "map.h"
#include <math.h>

void draw_3d_view(HWND hwnd, HDC hdc, Player p) {
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

    // 3. Lancer un rayon pour chaque colonne verticale de l'écran
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        // Calcul de l'angle du rayon par rapport au regard du joueur
        float ray_angle = (p.angle - FOV / 2.0f) + ((float)x / (float)SCREEN_WIDTH) * FOV;

        float distance_to_wall = 0.0f;
        bool hit_wall = false;

        float eye_x = cosf(ray_angle);
        float eye_y = sinf(ray_angle);

        // Faire avancer le rayon par petits pas
        while (!hit_wall && distance_to_wall < 16.0f) {
            distance_to_wall += 0.02f;

            int check_x = (int)(p.x + eye_x * distance_to_wall);
            int check_y = (int)(p.y + eye_y * distance_to_wall);

            // Si le rayon sort des limites de la carte
            if (check_x < 0 || check_x >= MAP_WIDTH || check_y < 0 || check_y >= MAP_HEIGHT) {
                hit_wall = true;
                distance_to_wall = 16.0f;
            } else if (game_map[check_y][check_x] == 1) {
                hit_wall = true;
            }
        }

        // Correction de l'effet "oeil de poisson" (fish-eye distortion)
        float corrected_dist = distance_to_wall * cosf(ray_angle - p.angle);
        if (corrected_dist < 0.1f) corrected_dist = 0.1f;

        // Calcul de la hauteur du mur à l'écran
        int wall_ceiling = (int)((float)(SCREEN_HEIGHT / 2.0f) - (float)SCREEN_HEIGHT / (corrected_dist * 1.5f));
        int wall_floor = SCREEN_HEIGHT - wall_ceiling;

        // Création d'un effet d'ombre : plus le mur est loin, plus il est sombre
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
    int size = 10; // Taille des branches du viseur

    HPEN crossPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0)); // Trait rouge de 2 pixels
    HPEN oldPen = (HPEN)SelectObject(hdc, crossPen);

    // Ligne horizontale
    MoveToEx(hdc, center_x - size, center_y, NULL);
    LineTo(hdc, center_x + size, center_y);

    // Ligne verticale
    MoveToEx(hdc, center_x, center_y - size, NULL);
    LineTo(hdc, center_x, center_y + size);

    SelectObject(hdc, oldPen);
    DeleteObject(crossPen);
}