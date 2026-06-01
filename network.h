#ifndef NETWORK_H
#define NETWORK_H

#include <winsock2.h>
#include <stdbool.h>
#include "player.h"

#define PORT 8888

// Types de paquets
#define PACKET_TYPE_NORMAL         0
#define PACKET_TYPE_ID_ASSIGN      1
#define PACKET_TYPE_DISCOVERY_REQ  2
#define PACKET_TYPE_DISCOVERY_RESP 3

// La structure des données qu'on va envoyer sur le réseau à chaque frame
typedef struct {
    int id;
    float x;
    float y;
    float angle;
    int hp;
    bool is_shooting;
    int packet_type;
} PlayerPacket;

// Structure pour la découverte LAN
typedef struct {
    int packet_type;
    char server_ip[64];
    int players_connected;
} DiscoveryPacket;

// Fonctions globales
bool init_networking();
int setup_udp_socket(bool is_server);
void enable_broadcast(int sock);
int discover_servers(int sock, char found_ips[][64], int max_servers);
void send_data(int sock, PlayerPacket packet, const char* target_ip);
bool receive_data(int sock, PlayerPacket* packet, struct sockaddr_in* from_addr);
void clean_networking();

#endif
