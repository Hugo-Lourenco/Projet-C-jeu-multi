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
Player Packets_players[5];

// Variables pour retenir l'ancienne fenêtre avant le plein écran
static WINDOWPLACEMENT wpPrev = { sizeof(wpPrev) };
bool is_fullscreen = false;
DWORD last_f11_time = 0;

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

    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);

    SetConsoleTitle("CSGROS Retro 3D - Launcher LAN");
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

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

    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("  [1] Heberger la partie (Serveur)\n");
    printf("  [2] Rejoindre une partie (Client)\n");

    printf("\n  > Votre choix : ");
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    int choix = 0;
    scanf("%d", &choix);

    // -----------------------------------------------------------------
    // ATTRIBUTION AUTOMATIQUE D'ID
    // Le serveur prend toujours l'ID 1.
    // Les clients reçoivent leur ID depuis le serveur (plus de saisie).
    // -----------------------------------------------------------------
    int mon_id = 0;

    char ip_serveur[64] = "127.0.0.1";
    if (choix == 2) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        printf("  > Entrez l'IP du serveur : ");
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        scanf("%s", ip_serveur);
    } else {
        // Le serveur est toujours le joueur 1
        mon_id = 1;
    }

    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    printf("\n  Initialisation du moteur graphique et du reseau ");
    for (int i = 0; i < 4; i++) { Sleep(350); printf("."); }
    printf("\n");

    init_textures();

    if (!init_networking()) { FreeConsole(); return 0; }
    int mon_socket = setup_udp_socket(choix == 1);
    if (mon_socket == -1) { clean_networking(); FreeConsole(); return 0; }

    // -----------------------------------------------------------------
    // CLIENT : demande d'ID automatique au serveur
    // -----------------------------------------------------------------
    if (choix == 2) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        printf("  Connexion au serveur %s, attribution de l'ID", ip_serveur);

        // Envoi de la demande d'ID (paquet spécial avec id=0)
        PlayerPacket demande = { 0, 0.0f, 0.0f, 0.0f, 0, false, PACKET_TYPE_ID_ASSIGN };
        send_data(mon_socket, demande, ip_serveur);

        bool id_recu = false;
        DWORD timeout_start = GetTickCount();

        while (!id_recu && GetTickCount() - timeout_start < 5000) {
            PlayerPacket rep;
            struct sockaddr_in from;
            if (receive_data(mon_socket, &rep, &from)) {
                if (rep.packet_type == PACKET_TYPE_ID_ASSIGN && rep.id >= 2 && rep.id <= 4) {
                    mon_id = rep.id;
                    id_recu = true;
                }
            }
            Sleep(50);
            printf(".");
        }
        printf("\n");

        if (!id_recu) {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
            printf("  ERREUR : Aucun ID recu du serveur.\n");
            printf("  Le serveur est peut-etre plein (4/4) ou inaccessible.\n");
            Sleep(3000);
            closesocket(mon_socket);
            clean_networking();
            FreeConsole();
            return 0;
        }

        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("  ID attribue par le serveur : %d\n", mon_id);
        Sleep(800);
    }

    FreeConsole();

    init_player(&p1, mon_id, 2.0f, 2.0f);

    const char CLASS_NAME[] = "CSGO_GDI_Class";
    WNDCLASS wc = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "CSGROS Retro 3D", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH, SCREEN_HEIGHT,
                               NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) { closesocket(mon_socket); clean_networking(); return 0; }

    ShowWindow(hwnd, nCmdShow);
    SetForegroundWindow(hwnd);

    MSG msg = {0};
    static DWORD last_shot_time = 0;
    struct sockaddr_in clients_addr[5];
    bool is_client_connected[5] = {false, false, false, false, false};

    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // --- GESTION DU PLEIN ÉCRAN (TOUCHE F11) ---
        if (window_focused && (GetAsyncKeyState(VK_F11) & 0x8000)) {
            if (GetTickCount() - last_f11_time > 500) {
                last_f11_time = GetTickCount();
                is_fullscreen = !is_fullscreen;

                DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
                if (is_fullscreen) {
                    MONITORINFO mi = { sizeof(mi) };
                    if (GetWindowPlacement(hwnd, &wpPrev) &&
                        GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
                        SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
                        SetWindowPos(hwnd, HWND_TOP,
                                     mi.rcMonitor.left, mi.rcMonitor.top,
                                     mi.rcMonitor.right  - mi.rcMonitor.left,
                                     mi.rcMonitor.bottom - mi.rcMonitor.top,
                                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                    }
                } else {
                    SetWindowLong(hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
                    SetWindowPlacement(hwnd, &wpPrev);
                    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                }
            }
        }

        if (!p1.is_alive) {
            if (window_focused && (GetAsyncKeyState('R') & 0x8000))
                init_player(&p1, p1.id, 2.0f, 2.0f);
        }

        bool shooting_now = false;

        if (p1.is_alive && window_focused) {
            RECT clRect;
            GetClientRect(hwnd, &clRect);
            POINT center_pt = { (clRect.right - clRect.left) / 2, (clRect.bottom - clRect.top) / 2 };
            ClientToScreen(hwnd, &center_pt);

            POINT mouse_pos;
            GetCursorPos(&mouse_pos);
            int delta_x = mouse_pos.x - center_pt.x;
            p1.angle += (float)delta_x * 0.0025f;
            SetCursorPos(center_pt.x, center_pt.y);

            float speed = 0.05f;
            if (GetAsyncKeyState('Z') & 0x8000) {
                float n_x = p1.x + cosf(p1.angle) * speed;
                float n_y = p1.y + sinf(p1.angle) * speed;
                if (game_map[(int)p1.y][(int)n_x] == 0) p1.x = n_x;
                if (game_map[(int)n_y][(int)p1.x] == 0) p1.y = n_y;
            }
            if (GetAsyncKeyState('S') & 0x8000) {
                float n_x = p1.x - cosf(p1.angle) * speed;
                float n_y = p1.y - sinf(p1.angle) * speed;
                if (game_map[(int)p1.y][(int)n_x] == 0) p1.x = n_x;
                if (game_map[(int)n_y][(int)p1.x] == 0) p1.y = n_y;
            }
            if (GetAsyncKeyState('Q') & 0x8000) {
                float s_a = p1.angle - 1.5708f;
                float n_x = p1.x + cosf(s_a) * speed;
                float n_y = p1.y + sinf(s_a) * speed;
                if (game_map[(int)p1.y][(int)n_x] == 0) p1.x = n_x;
                if (game_map[(int)n_y][(int)p1.x] == 0) p1.y = n_y;
            }
            if (GetAsyncKeyState('D') & 0x8000) {
                float s_a = p1.angle + 1.5708f;
                float n_x = p1.x + cosf(s_a) * speed;
                float n_y = p1.y + sinf(s_a) * speed;
                if (game_map[(int)p1.y][(int)n_x] == 0) p1.x = n_x;
                if (game_map[(int)n_y][(int)p1.x] == 0) p1.y = n_y;
            }

            if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                if (GetTickCount() - last_shot_time > 250) {
                    shooting_now = true;
                    last_shot_time = GetTickCount();
                }
            }
        }

        if (window_focused && (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) running = false;

        // --- RÉCEPTION RÉSEAU ---
        PlayerPacket paquet_recu;
        struct sockaddr_in adresse_provenance;
        while (receive_data(mon_socket, &paquet_recu, &adresse_provenance)) {

            // --- SERVEUR : attribution automatique d'ID à un nouveau client ---
            if (choix == 1 && paquet_recu.packet_type == PACKET_TYPE_ID_ASSIGN && paquet_recu.id == 0) {
                int free_id = -1;
                for (int i = 2; i <= 4; i++) {
                    if (!is_client_connected[i]) { free_id = i; break; }
                }
                if (free_id != -1) {
                    is_client_connected[free_id] = true;
                    clients_addr[free_id] = adresse_provenance;

                    // Répond au client avec son ID attribué
                    PlayerPacket rep = { free_id, 2.0f, 2.0f, 0.0f, 100, false, PACKET_TYPE_ID_ASSIGN };
                    sendto(mon_socket, (char*)&rep, sizeof(PlayerPacket), 0,
                           (struct sockaddr*)&adresse_provenance, sizeof(adresse_provenance));
                }
                continue; // Ce paquet est consommé, on passe au suivant
            }

            // --- Réception normale (paquets de jeu) ---
            if (choix == 1 && paquet_recu.id >= 1 && paquet_recu.id <= 4) {
                clients_addr[paquet_recu.id] = adresse_provenance;
                is_client_connected[paquet_recu.id] = true;
            }
            if (paquet_recu.id != p1.id && paquet_recu.id >= 1 && paquet_recu.id <= 4) {
                Packets_players[paquet_recu.id].x           = paquet_recu.x;
                Packets_players[paquet_recu.id].y           = paquet_recu.y;
                Packets_players[paquet_recu.id].angle       = paquet_recu.angle;
                Packets_players[paquet_recu.id].hp          = paquet_recu.hp;
                Packets_players[paquet_recu.id].is_alive    = (paquet_recu.hp > 0);
                Packets_players[paquet_recu.id].is_shooting = paquet_recu.is_shooting;

                if (paquet_recu.is_shooting && p1.is_alive) {
                    if (check_if_hit(paquet_recu, p1)) {
                        p1.hp -= 25;
                        if (p1.hp <= 0) { p1.hp = 0; p1.is_alive = false; }
                    }
                }
            }
        }

        // --- ENVOI RÉSEAU ---
        PlayerPacket mon_paquet = { p1.id, p1.x, p1.y, p1.angle, p1.hp, shooting_now, PACKET_TYPE_NORMAL };
        if (choix == 2) {
            send_data(mon_socket, mon_paquet, ip_serveur);
        } else if (choix == 1) {
            for (int i = 2; i <= 4; i++) {
                if (is_client_connected[i]) {
                    sendto(mon_socket, (char*)&mon_paquet, sizeof(PlayerPacket), 0,
                           (struct sockaddr*)&clients_addr[i], sizeof(clients_addr[i]));
                    // Relay des autres clients
                    for (int j = 2; j <= 4; j++) {
                        if (i != j && is_client_connected[j]) {
                            PlayerPacket relay_pkt = {
                                j,
                                Packets_players[j].x,
                                Packets_players[j].y,
                                Packets_players[j].angle,
                                Packets_players[j].hp,
                                Packets_players[j].is_shooting,
                                PACKET_TYPE_NORMAL
                            };
                            sendto(mon_socket, (char*)&relay_pkt, sizeof(PlayerPacket), 0,
                                   (struct sockaddr*)&clients_addr[i], sizeof(clients_addr[i]));
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

        draw_3d_view(hwnd, memDC, p1);

        if (!p1.is_alive) {
            SetTextColor(memDC, RGB(255, 0, 0));
            SetBkMode(memDC, TRANSPARENT);
            TextOut(memDC, SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 40, "YOU ARE DEAD", 12);
            TextOut(memDC, SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 + 65, "Appuyez sur R pour Respawn", 26);
        }

        // Étirement dynamique (pixel-art propre même en 1080p/4K)
        RECT clRect;
        GetClientRect(hwnd, &clRect);
        int win_w = clRect.right  - clRect.left;
        int win_h = clRect.bottom - clRect.top;

        SetStretchBltMode(hdc, COLORONCOLOR);
        StretchBlt(hdc, 0, 0, win_w, win_h, memDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SRCCOPY);

        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        ReleaseDC(hwnd, hdc);

        Sleep(16);
    }

    closesocket(mon_socket);
    clean_networking();
    ShowCursor(TRUE);
    return 0;
}
