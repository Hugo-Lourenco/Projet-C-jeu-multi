#ifndef NETWORK_H
#define NETWORK_H

#include <winsock2.h>
#include <stdbool.h>
#include "player.h"

#define PORT 8888 // Le port libre de notre choix pour le TP

// La structure des données qu'on va envoyer sur le réseau à chaque frame
typedef struct {
    int id;
    float x;
    float y;
    float angle;
    int hp;
    bool is_shooting;
} PlayerPacket;

// Fonctions globales
bool init_networking();
int setup_udp_socket(bool is_server);
void send_data(int sock, PlayerPacket packet, const char* target_ip);
bool receive_data(int sock, PlayerPacket* packet, struct sockaddr_in* from_addr);
void clean_networking();

#endif