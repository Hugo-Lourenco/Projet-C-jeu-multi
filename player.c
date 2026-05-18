#include "player.h"

void init_player(Player* p, int id, float start_x, float start_y) {
    p->id = id;
    p->x = start_x;
    p->y = start_y;
    p->angle = 0.0f; // Regarde vers la droite par défaut
    p->hp = 100;
    p->is_alive = true;
}