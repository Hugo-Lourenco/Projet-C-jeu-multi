#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

#include "player.h"

// Variable globale pour savoir si le jeu tourne
bool running = true;

// Fonction de gestion des messages de la fenêtre Windows
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
        case WM_DESTROY:
            running = false;
            PostQuitMessage(0);
            return 0;
            
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                running = false;
            }
            // Exemple : Appuyer sur ESPACE pour tirer
            if (wParam == VK_SPACE) {
                printf("Pan !\n");
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    // 1. Initialiser le joueur
    Player p1;
    init_player(&p1, 1, 100.0f, 100.0f); // Position en pixels pour l'instant

    // 2. Création de la fenêtre Windows
    const char CLASS_NAME[] = "CSGO_Class";
    WNDCLASS wc = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "CS:GO - Free For All (GDI)",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;
    ShowWindow(hwnd, nCmdShow);

    // 3. Boucle de jeu principale
    MSG msg = {0};
    while (running) {
        // A. Gestion des événements Windows (Clavier, Fermeture)
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // B. Logique du jeu et Réseau 
        // ... (Mise à jour des positions, etc.) ...

        // C. Rendu graphique avec GDI
        HDC hdc = GetDC(hwnd);
        
        // --- Effacer l'écran ---
        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, (HBRUSH) (COLOR_WINDOW+1));

        // --- Dessiner le joueur (un simple carré rouge pour commencer) ---
        HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
        RECT playerRect = { (int)p1.x, (int)p1.y, (int)p1.x + 50, (int)p1.y + 50 };
        FillRect(hdc, &playerRect, redBrush);
        DeleteObject(redBrush);

        ReleaseDC(hwnd, hdc);
        
        // Petite pause pour ne pas surcharger le processeur (simule environ 60 FPS)
        Sleep(16); 
    }

    return 0;
}