#include "network.h"
#include <stdio.h>

// Initialise la bibliothèque réseau de Windows (Winsock)
bool init_networking() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Erreur d'initialisation Winsock. Code : %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

// Crée et configure le socket UDP
int setup_udp_socket(bool is_server) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        printf("Impossible de créer le socket UDP.\n");
        return -1;
    }

    // Mode non-bloquant : pour que le jeu n'attende pas indéfiniment un message réseau
    unsigned long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    if (is_server) {
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY; // Écoute toutes les cartes réseau (Ethernet)
        server.sin_port = htons(PORT);

        if (bind(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
            printf("Erreur de liaison (bind). Code : %d\n", WSAGetLastError());
            return -1;
        }
    }
    return sock;
}

// Envoyer ses données
void send_data(int sock, PlayerPacket packet, const char* target_ip) {
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(PORT);
    target.sin_addr.s_addr = inet_addr(target_ip);

    sendto(sock, (char*)&packet, sizeof(PlayerPacket), 0, (struct sockaddr*)&target, sizeof(target));
}

// Recevoir les données d'un autre joueur (Non-bloquant)
bool receive_data(int sock, PlayerPacket* packet, struct sockaddr_in* from_addr) {
    int from_len = sizeof(struct sockaddr_in);
    int res = recvfrom(sock, (char*)packet, sizeof(PlayerPacket), 0, (struct sockaddr*)from_addr, &from_len);    
    if (res > 0) {
        return true; // Données reçues avec succès !
    }
    return false; // Pas de données disponibles à cet instant
}

void clean_networking() {
    WSACleanup();
}