#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

typedef struct {
    int id;
    float x;
    float y;
    float angle;
    int hp;
    bool is_alive;
    bool is_shooting; // <-- C'est l'information qui manquait !
} Player;

// Initialise un joueur avec son ID et sa position de départ
void init_player(Player* p, int id, float start_x, float start_y);

#endif