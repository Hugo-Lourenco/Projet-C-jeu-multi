#ifndef PLAYER_H
#define PLAYER_H
#include <stdbool.h>

// Structure définissant un joueur
typedef struct {
    int id;
    float x, y, z;
    int hp;
    bool is_alive;
} Player;

// Signatures des fonctions
void init_player(Player* p, int id, float start_x, float start_y);
void take_damage(Player* p, int damage);

#endif