#include "network.h"
#include <stdio.h>
#include <string.h>

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
        int reuse = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(PORT);

        if (bind(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
            printf("Erreur de liaison (bind). Code : %d\n", WSAGetLastError());
            return -1;
        }
    }
    return sock;
}

// Active le broadcast sur le socket
void enable_broadcast(int sock) {
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));
}

// Client : envoie un broadcast et collecte les réponses pendant 2 secondes
// Retourne le nombre de serveurs trouvés
int discover_servers(int sock, char found_ips[][64], int max_servers) {
    enable_broadcast(sock);

    DiscoveryPacket req;
    req.packet_type = PACKET_TYPE_DISCOVERY_REQ;
    req.players_connected = 0;
    memset(req.server_ip, 0, sizeof(req.server_ip));

    struct sockaddr_in bcast;
    bcast.sin_family = AF_INET;
    bcast.sin_port = htons(PORT);
    bcast.sin_addr.s_addr = inet_addr("255.255.255.255");

    sendto(sock, (char*)&req, sizeof(DiscoveryPacket), 0,
           (struct sockaddr*)&bcast, sizeof(bcast));

    int count = 0;
    DWORD start = GetTickCount();

    while (GetTickCount() - start < 2000 && count < max_servers) {
        DiscoveryPacket resp;
        struct sockaddr_in from;
        int from_len = sizeof(from);

        int res = recvfrom(sock, (char*)&resp, sizeof(DiscoveryPacket), 0,
                           (struct sockaddr*)&from, &from_len);

        if (res > 0 && resp.packet_type == PACKET_TYPE_DISCOVERY_RESP) {
            // Évite les doublons
            bool already = false;
            for (int i = 0; i < count; i++) {
                if (strcmp(found_ips[i], resp.server_ip) == 0) {
                    already = true;
                    break;
                }
            }
            if (!already) {
                strncpy(found_ips[count], resp.server_ip, 63);
                found_ips[count][63] = '\0';
                count++;
            }
        }
        Sleep(30);
    }
    return count;
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
        return true;
    }
    return false;
}

void clean_networking() {
    WSACleanup();
}
