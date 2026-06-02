#include "network.h"
#include <stdio.h>
#include <string.h>
#include <iphlpapi.h>

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

// Calcule les adresses de broadcast de toutes les interfaces réseau actives
// Retourne le nombre d'adresses trouvées
static int get_broadcast_addresses(unsigned long* bcast_addrs, int max_addrs) {
    int count = 0;

    // Toujours ajouter 255.255.255.255 en fallback
    bcast_addrs[count++] = inet_addr("255.255.255.255");

    // Récupérer les infos des adaptateurs réseau
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD bufLen = sizeof(adapterInfo);

    if (GetAdaptersInfo(adapterInfo, &bufLen) == NO_ERROR) {
        IP_ADAPTER_INFO* adapter = adapterInfo;
        while (adapter && count < max_addrs) {
            // Récupérer IP et masque
            unsigned long ip   = inet_addr(adapter->IpAddressList.IpAddress.String);
            unsigned long mask = inet_addr(adapter->IpAddressList.IpMask.String);

            // Ignorer les interfaces invalides ou loopback
            if (ip != 0 && ip != inet_addr("127.0.0.1") && mask != 0) {
                // Broadcast = (IP & masque) | (~masque)
                unsigned long bcast = (ip & mask) | (~mask);

                // Éviter les doublons
                bool already = false;
                for (int i = 0; i < count; i++) {
                    if (bcast_addrs[i] == bcast) { already = true; break; }
                }
                if (!already) {
                    printf("  Interface detectee : %s / %s -> broadcast %s\n",
                           adapter->IpAddressList.IpAddress.String,
                           adapter->IpAddressList.IpMask.String,
                           inet_ntoa(*(struct in_addr*)&bcast));
                    bcast_addrs[count++] = bcast;
                }
            }
            adapter = adapter->Next;
        }
    }
    return count;
}

// Client : envoie un broadcast sur toutes les interfaces et collecte les réponses
int discover_servers(int sock, char found_ips[][64], int max_servers) {
    enable_broadcast(sock);

    DiscoveryPacket req;
    req.packet_type = PACKET_TYPE_DISCOVERY_REQ;
    req.players_connected = 0;
    memset(req.server_ip, 0, sizeof(req.server_ip));

    // Envoyer le broadcast sur toutes les interfaces détectées
    unsigned long bcast_addrs[8];
    int nb_ifaces = get_broadcast_addresses(bcast_addrs, 8);

    for (int b = 0; b < nb_ifaces; b++) {
        struct sockaddr_in bcast;
        bcast.sin_family = AF_INET;
        bcast.sin_port = htons(PORT);
        bcast.sin_addr.s_addr = bcast_addrs[b];
        sendto(sock, (char*)&req, sizeof(DiscoveryPacket), 0,
               (struct sockaddr*)&bcast, sizeof(bcast));
    }

    int count = 0;
    DWORD start = GetTickCount();

    while (GetTickCount() - start < 2000 && count < max_servers) {
        DiscoveryPacket resp;
        struct sockaddr_in from;
        int from_len = sizeof(from);

        int res = recvfrom(sock, (char*)&resp, sizeof(DiscoveryPacket), 0,
                           (struct sockaddr*)&from, &from_len);

        if (res > 0 && resp.packet_type == PACKET_TYPE_DISCOVERY_RESP) {
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
