#define _WIN32_WINNT 0x0501 // Nécessaire pour certaines fonctions réseau anciennes sous MinGW
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "player.h"
#include "map.h"
#include "renderer.h"
#include "network.h"

bool running = true;
Player p1;
bool window_focused = true;

// Tableau pour stocker les positions des 4 joueurs de la partie
Player Packets_players[5]; 

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_ACTIVATE:
            window_focused = (LOWORD(wParam) != WA_INACTIVE);
            if (window_focused) {
                ShowCursor(FALSE); 
            } else {
                ShowCursor(TRUE);  
            }
            return 0;
            
        case WM_CLOSE:
        case WM_DESTROY:
            running = false;
            ShowCursor(TRUE); 
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    // --- MENU DE CONFIGURATION RÉSEAU (CONSOLE ANTÉRIEURE) ---
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);

    int choix = 0;
    char ip_serveur[64] = "127.0.0.1"; 
    int mon_id = 1;

    printf("=== CONFIGURATION RESEAU CS:GO GDI ===\n");
    printf("1. Heberger la partie (Serveur)\n");
    printf("2. Rejoindre une partie (Client)\n");
    printf("Choix : ");
    scanf("%d", &choix);

    printf("Entrez votre ID joueur unique (1, 2, 3 ou 4) : ");
    scanf("%d", &mon_id);

    if (choix == 2) {
        printf("Entrez l'IP de l'hebergeur (ex: 192.168.1.XX) : ");
        scanf("%s", ip_serveur);
    }

    // Fermeture de la console pour lancer la fenêtre de jeu
    FreeConsole();

    // --- INITIALISATION DU JOUEUR ET DU RÉSEAU ---
    init_player(&p1, mon_id, 2.0f, 2.0f);
    
    if (!init_networking()) {
        MessageBox(NULL, "Erreur d'initialisation réseau !", "Erreur", MB_OK);
        return 0;
    }
    
    int mon_socket = setup_udp_socket(choix == 1);
    if (mon_socket == -1) {
        MessageBox(NULL, "Erreur lors de la création du socket !", "Erreur", MB_OK);
        clean_networking();
        return 0;
    }

    // --- CRÉATION DE LA FENÊTRE WINDOWS ---
    const char CLASS_NAME[] = "CSGO_GDI_Class";
    WNDCLASS wc = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "CS:GO Retro 3D (GDI)",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH, SCREEN_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        closesocket(mon_socket);
        clean_networking();
        return 0;
    }
    
    ShowWindow(hwnd, nCmdShow);
    ShowCursor(FALSE); 

    MSG msg = {0};
    
    // --- BOUCLE DE JEU PRINCIPALE ---
    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // --- GESTION DE LA SOURIS (REGARD) ---
        if (window_focused) {
            POINT center_pt = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
            ClientToScreen(hwnd, &center_pt); 

            POINT mouse_pos;
            GetCursorPos(&mouse_pos);

            int delta_x = mouse_pos.x - center_pt.x;
            float sensitivity = 0.0025f; 
            p1.angle += (float)delta_x * sensitivity;

            SetCursorPos(center_pt.x, center_pt.y);
        }

        // --- GESTION DU CLAVIER (DEPLACEMENTS) ---
        float speed = 0.05f;

        // Avancer (Z)
        if (GetAsyncKeyState('Z') & 0x8000) {
            float new_x = p1.x + cosf(p1.angle) * speed;
            float new_y = p1.y + sinf(p1.angle) * speed;
            if (game_map[(int)p1.y][(int)new_x] == 0) p1.x = new_x;
            if (game_map[(int)new_y][(int)p1.x] == 0) p1.y = new_y;
        }
        // Reculer (S)
        if (GetAsyncKeyState('S') & 0x8000) {
            float new_x = p1.x - cosf(p1.angle) * speed;
            float new_y = p1.y - sinf(p1.angle) * speed;
            if (game_map[(int)p1.y][(int)new_x] == 0) p1.x = new_x;
            if (game_map[(int)new_y][(int)p1.x] == 0) p1.y = new_y;
        }
        // Strafe Gauche (Q)
        if (GetAsyncKeyState('Q') & 0x8000) {
            float strafe_angle = p1.angle - 1.5708f; 
            float new_x = p1.x + cosf(strafe_angle) * speed;
            float new_y = p1.y + sinf(strafe_angle) * speed;
            if (game_map[(int)p1.y][(int)new_x] == 0) p1.x = new_x;
            if (game_map[(int)new_y][(int)p1.x] == 0) p1.y = new_y;
        }
        // Strafe Droite (D)
        if (GetAsyncKeyState('D') & 0x8000) {
            float strafe_angle = p1.angle + 1.5708f; 
            float new_x = p1.x + cosf(strafe_angle) * speed;
            float new_y = p1.y + sinf(strafe_angle) * speed;
            if (game_map[(int)p1.y][(int)new_x] == 0) p1.x = new_x;
            if (game_map[(int)new_y][(int)p1.x] == 0) p1.y = new_y;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) running = false;

        // --- GESTION DU TIR ---
        bool shooting_now = false;
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            shooting_now = true;
            // On pourra ajouter l'effet sonore ou visuel ici
        }

        // --- ENVOI DES DONNÉES SUR LE RÉSEAU ---
        PlayerPacket mon_paquet = { p1.id, p1.x, p1.y, p1.angle, p1.hp, shooting_now };
        if (choix == 2) {
            // Si on est un client, on envoie nos infos à l'hôte
            send_data(mon_socket, mon_paquet, ip_serveur);
        }
        // Note : Dans un vrai switch local, le serveur envoie aussi ses données aux clients.
        // Pour les tests, envoyer à l'IP configurée suffit à lier les instances.

        // --- RÉCEPTION DES DONNÉES ENNEMIES ---
        PlayerPacket paquet_recu;
        struct sockaddr_in adresse_provenance;
        while (receive_data(mon_socket, &paquet_recu, &adresse_provenance)) {
            // Si le paquet vient d'un autre joueur, on met à jour notre connaissance de sa position
            if (paquet_recu.id != p1.id && paquet_recu.id >= 1 && paquet_recu.id <= 4) {
                Packets_players[paquet_recu.id].x = paquet_recu.x;
                Packets_players[paquet_recu.id].y = paquet_recu.y;
                Packets_players[paquet_recu.id].angle = paquet_recu.angle;
                Packets_players[paquet_recu.id].hp = paquet_recu.hp;
                Packets_players[paquet_recu.id].is_alive = (paquet_recu.hp > 0);
            }
        }

        // --- RENDU GRAPHIQUE (DOUBLE BUFFERING) ---
        HDC hdc = GetDC(hwnd);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, SCREEN_WIDTH, SCREEN_HEIGHT);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        // Dessin de la scène 3D
        draw_3d_view(hwnd, memDC, p1);

        // Projection de la mémoire vers l'écran visible
        BitBlt(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, memDC, 0, 0, SRCCOPY);

        // Nettoyage GDI
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        ReleaseDC(hwnd, hdc);

        Sleep(16); // ~60 FPS
    }

    // --- FERMETURE PROPRE ---
    closesocket(mon_socket);
    clean_networking();
    ShowCursor(TRUE);

    return 0;
}