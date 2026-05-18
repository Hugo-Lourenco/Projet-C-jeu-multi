#ifndef PLAYER_H
#define PLAYER_H
#include <stdbool.h>

typedef struct {
    int id;
    float x, y;     // Position sur la grille (ex: x=1.5, y=2.3)
    float angle;    // Direction du regard en radians
    int hp;
    bool is_alive;
} Player;

void init_player(Player* p, int id, float start_x, float start_y);

#endif