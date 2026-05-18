#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "player.h"
#include "map.h"
#include "renderer.h"

bool running = true;
Player p1;
bool window_focused = true; // Pour ne bloquer la souris que si on joue

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_ACTIVATE:
            // Si on clique en dehors de la fenêtre, on libère la souris
            window_focused = (LOWORD(wParam) != WA_INACTIVE);
            if (window_focused) {
                ShowCursor(FALSE); // Cache la souris en jeu
            } else {
                ShowCursor(TRUE);  // Réaffiche la souris si on ALT-TAB
            }
            return 0;
            
        case WM_CLOSE:
        case WM_DESTROY:
            running = false;
            ShowCursor(TRUE); // Remet la souris à la fermeture
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    init_player(&p1, 1, 2.0f, 2.0f);

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

    if (hwnd == NULL) return 0;
    ShowWindow(hwnd, nCmdShow);
    ShowCursor(FALSE); // Cache le curseur au démarrage

    MSG msg = {0};
    
    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // --- GESTION DE LA SOURIS (REGARD) ---
        if (window_focused) {
            POINT center_pt = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
            ClientToScreen(hwnd, &center_pt); // Coordonnées absolues de l'écran

            POINT mouse_pos;
            GetCursorPos(&mouse_pos);

            // Calcul de l'écart avec le centre
            int delta_x = mouse_pos.x - center_pt.x;

            // Sensibilité de la souris (ajuste cette valeur si c'est trop rapide)
            float sensitivity = 0.0025f; 
            p1.angle += (float)delta_x * sensitivity;

            // On force la souris à revenir au centre
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
            float strafe_angle = p1.angle - 1.5708f; // -90 degrés
            float new_x = p1.x + cosf(strafe_angle) * speed;
            float new_y = p1.y + sinf(strafe_angle) * speed;
            if (game_map[(int)p1.y][(int)new_x] == 0) p1.x = new_x;
            if (game_map[(int)new_y][(int)p1.x] == 0) p1.y = new_y;
        }
        // Strafe Droite (D)
        if (GetAsyncKeyState('D') & 0x8000) {
            float strafe_angle = p1.angle + 1.5708f; // +90 degrés
            float new_x = p1.x + cosf(strafe_angle) * speed;
            float new_y = p1.y + sinf(strafe_angle) * speed;
            if (game_map[(int)p1.y][(int)new_x] == 0) p1.x = new_x;
            if (game_map[(int)new_y][(int)p1.x] == 0) p1.y = new_y;
        }

        // --- TIRER (CLIC GAUCHE DE LA SOURIS) ---
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            printf("PAN ! Le joueur %d a tire au clic !\n", p1.id);
            Sleep(120); // Cadence de tir
        }

        // Quitter (ECHAP)
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) running = false;

        // --- RENDU GRAPHIQUE ---
        HDC hdc = GetDC(hwnd);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, SCREEN_WIDTH, SCREEN_HEIGHT);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        draw_3d_view(hwnd, memDC, p1);

        BitBlt(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        ReleaseDC(hwnd, hdc);

        Sleep(16); 
    }

    return 0;
}   