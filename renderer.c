#include "renderer.h"
#include "map.h"
#include <math.h>
#include <stdio.h>

// On récupère le tableau des joueurs mis à jour en continu par le réseau
extern Player Packets_players[5];

void draw_3d_view(HWND hwnd, HDC hdc, Player p) {
    // Chronomètre pour l'effacement de l'ATH tuto au bout de 10s
    static DWORD start_time = 0;
    if (start_time == 0) {
        start_time = GetTickCount();
    }

    // Le Z-Buffer pour stocker la distance des murs sur chaque colonne verticale de l'écran
    float z_buffer[SCREEN_WIDTH];

    // 1. DESSINER LE CIEL (Moitié supérieure de l'écran en bleu/gris)
    HBRUSH skyBrush = CreateSolidBrush(RGB(50, 60, 70));
    RECT skyRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2 };
    FillRect(hdc, &skyRect, skyBrush);
    DeleteObject(skyBrush);

    // 2. DESSINER LE SOL (Moitié inférieure en gris foncé)
    HBRUSH floorBrush = CreateSolidBrush(RGB(30, 30, 30));
    RECT floorRect = { 0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT };
    FillRect(hdc, &floorRect, floorBrush);
    DeleteObject(floorBrush);

    // 3. RENDU DES MURS (Moteur Raycasting)
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        // Calcul de l'angle du rayon pour cette colonne précise
        float ray_angle = (p.angle - FOV / 2.0f) + ((float)x / (float)SCREEN_WIDTH) * FOV;
        float distance_to_wall = 0.0f;
        bool hit_wall = false;

        float eye_x = cosf(ray_angle);
        float eye_y = sinf(ray_angle);

        // On fait avancer le rayon par petits pas jusqu'à taper un mur (1)
        while (!hit_wall && distance_to_wall < 16.0f) {
            distance_to_wall += 0.02f;
            int check_x = (int)(p.x + eye_x * distance_to_wall);
            int check_y = (int)(p.y + eye_y * distance_to_wall);

            // Hors limites ou collision mur
            if (check_x < 0 || check_x >= MAP_WIDTH || check_y < 0 || check_y >= MAP_HEIGHT) {
                hit_wall = true;
                distance_to_wall = 16.0f;
            } else if (game_map[check_y][check_x] == 1) {
                hit_wall = true;
            }
        }

        // Correction de l'effet distorsion "oeil de poisson"
        float corrected_dist = distance_to_wall * cosf(ray_angle - p.angle);
        if (corrected_dist < 0.1f) corrected_dist = 0.1f;

        // On remplit le Z-Buffer pour cette colonne
        z_buffer[x] = corrected_dist;

        // Calcul des coordonnées de la ligne verticale du mur à dessiner
        int wall_ceiling = (int)((float)(SCREEN_HEIGHT / 2.0f) - (float)SCREEN_HEIGHT / (corrected_dist * 1.5f));
        int wall_floor = SCREEN_HEIGHT - wall_ceiling;

        // Effet d'ombrage avec la distance (plus c'est loin, plus c'est noir)
        int color_val = (int)(255.0f / (1.0f + corrected_dist * corrected_dist * 0.1f));
        if (color_val > 255) color_val = 255;
        if (color_val < 0) color_val = 0;

        HBRUSH wallBrush = CreateSolidBrush(RGB(color_val, color_val, color_val));
        RECT wallColumn = { x, wall_ceiling, x + 1, wall_floor };
        FillRect(hdc, &wallColumn, wallBrush);
        DeleteObject(wallBrush);
    }

    // 4. RENDU DES ADVERSAIRES (Les rectangles rouges)
    for (int i = 1; i <= 4; i++) {
        // Conditions : ne pas se dessiner soi-même, vérifier que le joueur est connecté (x != 0) et VIVANT
        if (i == p.id || Packets_players[i].x == 0 || !Packets_players[i].is_alive) continue;

        // Vecteur de distance
        float sprite_x = Packets_players[i].x - p.x;
        float sprite_y = Packets_players[i].y - p.y;
        
        // Angle de l'ennemi par rapport à notre orientation de regard
        float sprite_angle = atan2f(sprite_y, sprite_x) - p.angle;

        // Normalisation de l'angle
        while (sprite_angle < -3.14159f) sprite_angle += 2.0f * 3.14159f;
        while (sprite_angle > 3.14159f)  sprite_angle -= 2.0f * 3.14159f;

        float sprite_dist = sqrtf(sprite_x * sprite_x + sprite_y * sprite_y);
        if (sprite_dist < 0.1f) sprite_dist = 0.1f;

        // Si l'adversaire est dans notre champ de vision
        if (fabs(sprite_angle) < FOV) {
            int sprite_screen_x = (int)((SCREEN_WIDTH / 2) + (sprite_angle / FOV) * SCREEN_WIDTH);
            
            // Calcul de la taille du rectangle de l'adversaire selon sa distance
            int sprite_size = (int)(SCREEN_HEIGHT / (sprite_dist * 1.5f));
            int sprite_ceil = SCREEN_HEIGHT / 2 - sprite_size / 2;
            int sprite_floor = SCREEN_HEIGHT / 2 + sprite_size / 2;

            int start_x = sprite_screen_x - sprite_size / 4;
            int end_x = sprite_screen_x + sprite_size / 4;

            if (start_x < 0) start_x = 0;
            if (end_x >= SCREEN_WIDTH) end_x = SCREEN_WIDTH - 1;

            HBRUSH enemyBrush = CreateSolidBrush(RGB(220, 50, 50));

            // Dessin colonne par colonne en comparant avec le Z-Buffer des murs
            for (int col = start_x; col <= end_x; col++) {
                // S'il est plus proche que le mur sur cette colonne, on l'affiche !
                if (sprite_dist < z_buffer[col]) {
                    RECT enemyColumn = { col, sprite_ceil, col + 1, sprite_floor };
                    FillRect(hdc, &enemyColumn, enemyBrush);
                }
            }
            DeleteObject(enemyBrush);
        }
    }

    // 5. DESSINER LE VISEUR (CROSSHAIR) AU CENTRE
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

    // 6. ATH - BARRE DE VIE (EN BAS À GAUCHE)
    int bar_x = 20;
    int bar_y = SCREEN_HEIGHT - 65; 
    int bar_width = 150;
    int bar_height = 15;

    // Fond gris de la barre
    HBRUSH bgBrush = CreateSolidBrush(RGB(40, 40, 40));
    RECT bgRect = { bar_x, bar_y, bar_x + bar_width, bar_y + bar_height };
    FillRect(hdc, &bgRect, bgBrush);
    DeleteObject(bgBrush);

    // Remplissage dynamique
    int current_width = (int)(((float)p.hp / 100.0f) * bar_width);
    if (current_width < 0) current_width = 0;
    if (current_width > bar_width) current_width = bar_width;

    // Changement de couleur (Vert si en forme, rouge si < 30 PV)
    HBRUSH hpBrush;
    if (p.hp > 30) {
        hpBrush = CreateSolidBrush(RGB(46, 204, 113)); 
    } else {
        hpBrush = CreateSolidBrush(RGB(231, 76, 60));  
    }
    RECT hpRect = { bar_x, bar_y, bar_x + current_width, bar_y + bar_height };
    FillRect(hdc, &hpRect, hpBrush);
    DeleteObject(hpBrush);

    // Texte numérique "HP: XX"
    char hp_text[16];
    sprintf(hp_text, "HP: %d", p.hp);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, bar_x + bar_width + 10, bar_y - 1, hp_text, strlen(hp_text));

    // 7. ATH - TEXTE DE TUTORIEL (Affiché pendant les 10 premières secondes)
    if (GetTickCount() - start_time < 10000) {
        SetTextColor(hdc, RGB(0, 255, 0));
        TextOut(hdc, 15, 15,  "--- TUTORIEL DE JEU ---", 23);
        TextOut(hdc, 15, 40,  "Z : Avancer", 11);
        TextOut(hdc, 15, 60,  "S : Reculer", 11);
        TextOut(hdc, 15, 80,  "Q : Strafe Gauche", 17);
        TextOut(hdc, 15, 100, "D : Strafe Droite", 17);
        TextOut(hdc, 15, 125, "Souris : Tourner la camera", 26);
        TextOut(hdc, 15, 145, "Clic Gauche : Tirer", 19);
        TextOut(hdc, 15, 170, "Echap : Quitter le jeu", 22);
    }
}