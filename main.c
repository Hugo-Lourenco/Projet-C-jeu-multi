#define _WIN32_WINNT 0x0501
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

// Tableau pour stocker les infos de tout le monde
Player Packets_players[5]; 

// Fonction de détection des tirs (Hitscan)
bool check_if_hit(PlayerPacket shooter, Player target) {
    float dx = target.x - shooter.x;
    float dy = target.y - shooter.y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist > 16.0f) return false;

    float angle_to_target = atan2f(dy, dx);
    float diff = angle_to_target - shooter.angle;
    
    while (diff < -3.14159f) diff += 2.0f * 3.14159f;
    while (diff > 3.14159f)  diff -= 2.0f * 3.14159f;

    float tolerance = 0.25f / dist; 
    if (tolerance > 0.4f) tolerance = 0.4f;

    if (fabs(diff) < tolerance) {
        float step = 0.1f;
        float check_dist = 0.1f;
        while (check_dist < dist) {
            int cx = (int)(shooter.x + cosf(angle_to_target) * check_dist);
            int cy = (int)(shooter.y + sinf(angle_to_target) * check_dist);
            if (game_map[cy][cx] == 1) return false; 
            check_dist += step;
        }
        return true; 
    }
    return false;
}

// Gestion des événements de la fenêtre Windows
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_ACTIVATE:
            window_focused = (LOWORD(wParam) != WA_INACTIVE);
            if (window_focused) ShowCursor(FALSE); 
            else ShowCursor(TRUE);  
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
    
    // --- MENU DE CONFIGURATION RÉSEAU (CONSOLE STYLÉE) ---
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);

    SetConsoleTitle("CSGROS Retro 3D - Launcher LAN");
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // Nouveau Logo CSGROS en ASCII Art (Couleur Vert Fluo)
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    printf("\n");
    printf("  ========================================================\n");
    printf("      _____   _____    _____  _____     ____    _____     \n");
    printf("     / ____| / ____|  / ____||  __ \\   / __ \\  / ____|    \n");
    printf("    | |     | (___   | |  __ | |__) | | |  | || (___      \n");
    printf("    | |      \\___ \\  | | |_ ||  _  /  | |  | | \\___ \\     \n");
    printf("    | |____  ____) | | |__| || | \\ \\  | |__| | ____) |    \n");
    printf("     \\_____||_____/   \\_____||_|  \\_\\  \\____/ |_____/     \n");
    printf("  ========================================================\n");
    printf("                     L A N   L A U N C H E R              \n\n");

    // Les menus (Couleur Blanc brillant)
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); 
    printf("  [1] Heberger la partie (Serveur)\n");
    printf("  [2] Rejoindre une partie (Client)\n");
    
    printf("\n  > Votre choix : ");
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY); 
    
    int choix = 0;
    scanf("%d", &choix);

    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("  > Entrez votre ID unique (1, 2, 3 ou 4) : ");
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    
    int mon_id = 1;
    scanf("%d", &mon_id);

    char ip_serveur[64] = "127.0.0.1"; 
    if (choix == 2) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        printf("  > Entrez l'IP du serveur : ");
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        scanf("%s", ip_serveur);
    }

    // Petite animation de chargement stylée
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); 
    printf("\n  Initialisation du moteur graphique et du reseau ");
    for(int i = 0; i < 4; i++) {
        Sleep(350); 
        printf(".");
    }
    printf("\n");

    FreeConsole(); // On détruit la console, place au jeu graphique !
    
    // --- INITIALISATION DU NOUVEAU MOTEUR GRAPHIQUE ---
    init_textures(); 

    // --- INITIALISATIONS JOUEUR ET RÉSEAU ---
    init_player(&p1, mon_id, 2.0f, 2.0f);
    if (!init_networking()) return 0;
    int mon_socket = setup_udp_socket(choix == 1);
    if (mon_socket == -1) { clean_networking(); return 0; }

    // --- CRÉATION DE LA FENÊTRE WINDOWS ---
    const char CLASS_NAME[] = "CSGO_GDI_Class";
    WNDCLASS wc = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "CSGROS Retro 3D (GDI)", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH, SCREEN_HEIGHT, NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) { closesocket(mon_socket); clean_networking(); return 0; }
    
    ShowWindow(hwnd, nCmdShow);
    SetForegroundWindow(hwnd); // Force le focus sur le jeu direct !

    MSG msg = {0};
    static DWORD last_shot_time = 0; 

    // Le registre des adresses clients (utilisé par le serveur pour le relais)
    struct sockaddr_in clients_addr[5];
    bool is_client_connected[5] = {false, false, false, false, false};

    // --- BOUCLE PRINCIPALE DE JEU ---
    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // --- RESPAWN (SI MORT) ---
        if (!p1.is_alive) {
            if (window_focused && (GetAsyncKeyState('R') & 0x8000)) {
                init_player(&p1, p1.id, 2.0f, 2.0f);
            }
        }

        bool shooting_now = false;
        
        // --- CONTRÔLES (SI VIVANT ET FENÊTRE ACTIVE) ---
        if (p1.is_alive && window_focused) {
            // Souris (Rotation)
            POINT center_pt = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
            ClientToScreen(hwnd, &center_pt); 
            POINT mouse_pos;
            GetCursorPos(&mouse_pos);
            int delta_x = mouse_pos.x - center_pt.x;
            p1.angle += (float)delta_x * 0.0025f;
            SetCursorPos(center_pt.x, center_pt.y);

            // Déplacements ZQSD
            float speed = 0.05f;
            if (GetAsyncKeyState('Z') & 0x8000) {
                float n_x = p1.x + cosf(p1.angle) * speed; float n_y = p1.y + sinf(p1.angle) * speed;
                if (game_map[(int)p1.y][(int)n_x] == 0) p1.x = n_x; if (game_map[(int)n_y][(int)p1.x] == 0) p1.y = n_y;
            }
            if (GetAsyncKeyState('S') & 0x8000) {
                float n_x = p1.x - cosf(p1.angle) * speed; float n_y = p1.y - sinf(p1.angle) * speed;
                if (game_map[(int)p1.y][(int)n_x] == 0) p1.x = n_x; if (game_map[(int)n_y][(int)p1.x] == 0) p1.y = n_y;
            }
            if (GetAsyncKeyState('Q') & 0x8000) {
                float s_a = p1.angle - 1.5708f; float n_x = p1.x + cosf(s_a) * speed; float n_y = p1.y + sinf(s_a) * speed;
                if (game_map[(int)p1.y][(int)n_x] == 0) p1.x = n_x; if (game_map[(int)n_y][(int)p1.x] == 0) p1.y = n_y;
            }
            if (GetAsyncKeyState('D') & 0x8000) {
                float s_a = p1.angle + 1.5708f; float n_x = p1.x + cosf(s_a) * speed; float n_y = p1.y + sinf(s_a) * speed;
                if (game_map[(int)p1.y][(int)n_x] == 0) p1.x = n_x; if (game_map[(int)n_y][(int)p1.x] == 0) p1.y = n_y;
            }

            // Tir (Clic Gauche)
            if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                if (GetTickCount() - last_shot_time > 250) {
                    shooting_now = true;
                    last_shot_time = GetTickCount();
                }
            }
        }

        if (window_focused && (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) running = false;

        // --- RÉCEPTION ET DÉTECTION DE HIT ---
        PlayerPacket paquet_recu;
        struct sockaddr_in adresse_provenance;
        while (receive_data(mon_socket, &paquet_recu, &adresse_provenance)) {
            
            // Le Serveur enregistre l'adresse de chaque client connecté
            if (choix == 1 && paquet_recu.id >= 1 && paquet_recu.id <= 4) {
                clients_addr[paquet_recu.id] = adresse_provenance;
                is_client_connected[paquet_recu.id] = true;
            }

            if (paquet_recu.id != p1.id && paquet_recu.id >= 1 && paquet_recu.id <= 4) {
                Packets_players[paquet_recu.id].x = paquet_recu.x;
                Packets_players[paquet_recu.id].y = paquet_recu.y;
                Packets_players[paquet_recu.id].angle = paquet_recu.angle;
                Packets_players[paquet_recu.id].hp = paquet_recu.hp;
                Packets_players[paquet_recu.id].is_alive = (paquet_recu.hp > 0);
                Packets_players[paquet_recu.id].is_shooting = paquet_recu.is_shooting; 

                // Si un ennemi tire et qu'on est vivant, on vérifie s'il nous touche !
                if (paquet_recu.is_shooting && p1.is_alive) {
                    if (check_if_hit(paquet_recu, p1)) {
                        p1.hp -= 25; 
                        if (p1.hp <= 0) { p1.hp = 0; p1.is_alive = false; }
                    }
                }
            }
        }

        // --- ENVOI ET RELAIS RÉSEAU ---
        PlayerPacket mon_paquet = { p1.id, p1.x, p1.y, p1.angle, p1.hp, shooting_now };
        
        if (choix == 2) {
            // Le Client arrose directement le Serveur
            send_data(mon_socket, mon_paquet, ip_serveur);
        } 
        else if (choix == 1) {
            // Le Serveur centralise et distribue à tout le monde
            for (int i = 2; i <= 4; i++) {
                if (is_client_connected[i]) {
                    // 1. Envoie ses propres données au client 'i'
                    sendto(mon_socket, (char*)&mon_paquet, sizeof(PlayerPacket), 0, (struct sockaddr*)&clients_addr[i], sizeof(clients_addr[i]));
                    
                    // 2. Relie les données de tous les autres clients vers 'i'
                    for (int j = 2; j <= 4; j++) {
                        if (i != j && is_client_connected[j]) {
                            PlayerPacket relay_pkt = { 
                                j, Packets_players[j].x, Packets_players[j].y, 
                                Packets_players[j].angle, Packets_players[j].hp, Packets_players[j].is_shooting 
                            };
                            sendto(mon_socket, (char*)&relay_pkt, sizeof(PlayerPacket), 0, (struct sockaddr*)&clients_addr[i], sizeof(clients_addr[i]));
                        }
                    }
                }
            }
        }

        // --- GRAPHISMES ---
        HDC hdc = GetDC(hwnd);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, SCREEN_WIDTH, SCREEN_HEIGHT);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        // Appel du moteur de rendu (Textures ou Solides selon ton renderer.c)
        draw_3d_view(hwnd, memDC, p1);

        // Overlay d'écran de mort
        if (!p1.is_alive) {
            SetTextColor(memDC, RGB(255, 0, 0));
            SetBkMode(memDC, TRANSPARENT);
            TextOut(memDC, SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 40, "YOU ARE DEAD", 12);
            TextOut(memDC, SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 + 65, "Appuyez sur R pour Respawn", 26);
        }

        BitBlt(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBitmap); DeleteObject(memBitmap); DeleteDC(memDC); ReleaseDC(hwnd, hdc);

        Sleep(16); // ~60 FPS
    }

    // --- FERMETURE PROPRE ---
    closesocket(mon_socket); clean_networking(); ShowCursor(TRUE);
    return 0;
}