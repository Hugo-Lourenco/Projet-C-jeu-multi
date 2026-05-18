#ifndef MAP_H
#define MAP_H
#include <stdbool.h>

#define MAP_WIDTH 20
#define MAP_HEIGHT 20

// Tableau 2D représentant la carte (0 = vide, 1 = mur)
extern int game_map[MAP_HEIGHT][MAP_WIDTH];

// Fonction pour vérifier si un joueur fonce dans un mur
bool check_collision(float x, float y);

#endif