#include "renderer.h"
#include "map.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef int GpStatus;
#define Ok 0
#ifndef WINGDIPAPI
#define WINGDIPAPI __stdcall
#endif

    typedef float REAL;
    typedef int GpUnit;
    typedef enum GpMatrixOrder
    {
        MatrixOrderPrepend = 0,
        MatrixOrderAppend = 1
    } GpMatrixOrder;

    typedef void GpBitmap;
    typedef void GpGraphics;

    typedef struct GdiplusStartupInput
    {
        UINT32 GdiplusVersion;
        void *DebugEventCallback;
        BOOL SuppressBackgroundThread;
        BOOL SuppressExternalCodecs;
    } GdiplusStartupInput;

    GpStatus WINGDIPAPI GdiplusStartup(ULONG_PTR *token, const GdiplusStartupInput *input, void *output);
    GpStatus WINGDIPAPI GdiplusShutdown(ULONG_PTR token);
    GpStatus WINGDIPAPI GdipCreateBitmapFromFile(const WCHAR *filename, GpBitmap **bitmap);
    GpStatus WINGDIPAPI GdipCreateFromHDC(HDC hdc, GpGraphics **graphics);
    GpStatus WINGDIPAPI GdipDrawImageRectI(GpGraphics *graphics, GpBitmap *image, INT x, INT y, INT width, INT height);
    GpStatus WINGDIPAPI GdipDrawImageRectRectI(GpGraphics *graphics, GpBitmap *image, INT dstX, INT dstY, INT dstWidth, INT dstHeight, INT srcX, INT srcY, INT srcWidth, INT srcHeight, GpUnit srcUnit, void *imageAttributes, void *callback, void *callbackData);
    GpStatus WINGDIPAPI GdipDeleteGraphics(GpGraphics *graphics);
    GpStatus WINGDIPAPI GdipDisposeImage(GpBitmap *bitmap);
    GpStatus WINGDIPAPI GdipTranslateWorldTransform(GpGraphics *graphics, REAL dx, REAL dy, GpMatrixOrder order);
    GpStatus WINGDIPAPI GdipRotateWorldTransform(GpGraphics *graphics, REAL angle, GpMatrixOrder order);

#ifdef __cplusplus
}
#endif

extern Player Packets_players[5];

static ULONG_PTR gdiPlusToken = 0;
static GpBitmap *weaponImageNormal = NULL;
static GpBitmap *weaponImageFire1 = NULL;
static GpBitmap *weaponImageFire2 = NULL;

static bool load_weapon_bitmap(const WCHAR *suffix, GpBitmap **bitmap)
{
    WCHAR exePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0)
        return false;
    WCHAR *lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash)
        *lastSlash = L'\0';

    if (wcslen(exePath) + wcslen(suffix) + 1 >= MAX_PATH)
        return false;
    wcscat(exePath, suffix);

    if (GdipCreateBitmapFromFile(exePath, bitmap) != Ok)
        return false;

    return true;
}

uint32_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
uint32_t tex_wall[64 * 64];
uint32_t tex_enemy[64 * 64];

uint32_t get_color(uint8_t r, uint8_t g, uint8_t b)
{
    return (r << 16) | (g << 8) | b;
}

bool init_weapon_asset()
{
    if (gdiPlusToken == 0)
    {
        GdiplusStartupInput gsi;
        gsi.GdiplusVersion = 1;
        gsi.DebugEventCallback = NULL;
        gsi.SuppressBackgroundThread = FALSE;
        gsi.SuppressExternalCodecs = FALSE;
        if (GdiplusStartup(&gdiPlusToken, &gsi, NULL) != Ok)
            return false;
    }

    if (!load_weapon_bitmap(L"\\asset\\m4 neutre.png", &weaponImageNormal))
        return false;
    if (!load_weapon_bitmap(L"\\asset\\m4_tir_detoure.png", &weaponImageFire1))
        return false;
    if (!load_weapon_bitmap(L"\\asset\\m4_tir2_detoure.png", &weaponImageFire2))
        return false;

    return true;
}

void cleanup_weapon_asset()
{
    if (weaponImageNormal)
    {
        GdipDisposeImage(weaponImageNormal);
        weaponImageNormal = NULL;
    }
    if (weaponImageFire1)
    {
        GdipDisposeImage(weaponImageFire1);
        weaponImageFire1 = NULL;
    }
    if (weaponImageFire2)
    {
        GdipDisposeImage(weaponImageFire2);
        weaponImageFire2 = NULL;
    }
    if (gdiPlusToken != 0)
    {
        GdiplusShutdown(gdiPlusToken);
        gdiPlusToken = 0;
    }
}

void draw_weapon_asset(HDC hdc, int x, int y, int width, int height, int frame, int recoil)
{
    GpBitmap *image = weaponImageNormal;
    if (frame == 1)
        image = weaponImageFire1;
    else if (frame == 2)
        image = weaponImageFire2;

    if (!image)
        return;

    GpGraphics *graphics = NULL;
    if (GdipCreateFromHDC(hdc, &graphics) != Ok)
        return;

    if (recoil > 0)
    {
        int srcY = recoil;
        int srcHeight = height - recoil;
        int destY = y + recoil;
        GdipDrawImageRectRectI(graphics, image, x, destY, width, srcHeight, 0, srcY, width, srcHeight, 2, NULL, NULL, NULL);
    }
    else
    {
        GdipDrawImageRectI(graphics, image, x, y, width, height);
    }

    GdipDeleteGraphics(graphics);
}

void init_textures()
{
    for (int y = 0; y < 64; y++)
    {
        for (int x = 0; x < 64; x++)
        {
            if (y % 32 == 0 || y % 32 == 31 || (y < 32 && x % 32 == 0) || (y >= 32 && (x + 16) % 32 == 0))
            {
                tex_wall[y * 64 + x] = get_color(80, 80, 80);
            }
            else
            {
                int grain = (x * y) % 20;
                tex_wall[y * 64 + x] = get_color(180 - grain, 50 - grain / 2, 50 - grain / 2);
            }
        }
    }

    for (int y = 0; y < 64; y++)
    {
        for (int x = 0; x < 64; x++)
        {
            tex_enemy[y * 64 + x] = 0;
            float cx = x - 32;
            if (y > 10 && y < 25 && cx * cx + (y - 17) * (y - 17) < 60)
                tex_enemy[y * 64 + x] = get_color(220, 180, 150);
            if (y > 14 && y < 19 && x > 25 && x < 45)
                tex_enemy[y * 64 + x] = get_color(255, 0, 0);
            if (y >= 25 && y < 55 && x > 15 && x < 49)
                tex_enemy[y * 64 + x] = get_color(40, 40, 40);
            if (y >= 55 && ((x > 15 && x < 28) || (x > 36 && x < 49)))
                tex_enemy[y * 64 + x] = get_color(30, 30, 30);
        }
    }
}

void draw_3d_view(HWND hwnd, HDC hdc, Player p)
{
    (void)hwnd;
    float z_buffer[SCREEN_WIDTH];

    for (int y = 0; y < SCREEN_HEIGHT / 2; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
            framebuffer[y * SCREEN_WIDTH + x] = get_color(50, 60, 70);

    for (int y = SCREEN_HEIGHT / 2; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
            framebuffer[y * SCREEN_WIDTH + x] = get_color(30, 30, 30);

    for (int x = 0; x < SCREEN_WIDTH; x++)
    {
        float ray_angle = (p.angle - FOV / 2.0f) + ((float)x / (float)SCREEN_WIDTH) * FOV;
        float distance_to_wall = 0.0f;
        bool hit_wall = false;

        float eye_x = cosf(ray_angle);
        float eye_y = sinf(ray_angle);

        while (!hit_wall && distance_to_wall < 16.0f)
        {
            distance_to_wall += 0.02f;
            int check_x = (int)(p.x + eye_x * distance_to_wall);
            int check_y = (int)(p.y + eye_y * distance_to_wall);

            if (check_x < 0 || check_x >= MAP_WIDTH || check_y < 0 || check_y >= MAP_HEIGHT)
            {
                hit_wall = true;
                distance_to_wall = 16.0f;
            }
            else if (game_map[check_y][check_x] == 1)
            {
                hit_wall = true;
            }
        }

        float corrected_dist = distance_to_wall * cosf(ray_angle - p.angle);
        if (corrected_dist < 0.1f)
            corrected_dist = 0.1f;
        z_buffer[x] = corrected_dist;

        float exact_hit_x = p.x + eye_x * distance_to_wall;
        float exact_hit_y = p.y + eye_y * distance_to_wall;
        float frac_x = exact_hit_x - floorf(exact_hit_x);
        float frac_y = exact_hit_y - floorf(exact_hit_y);

        float diff_x = frac_x < 0.5f ? frac_x : 1.0f - frac_x;
        float diff_y = frac_y < 0.5f ? frac_y : 1.0f - frac_y;

        int tex_x;
        bool is_dark = false;
        if (diff_x < diff_y)
        {
            tex_x = (int)(frac_y * 64.0f);
            is_dark = true;
        }
        else
        {
            tex_x = (int)(frac_x * 64.0f);
        }
        if (tex_x < 0)
            tex_x = 0;
        if (tex_x > 63)
            tex_x = 63;

        int wall_height = (int)((float)SCREEN_HEIGHT / (corrected_dist * 1.5f));
        int wall_ceiling = (SCREEN_HEIGHT / 2) - (wall_height / 2);
        int wall_floor = (SCREEN_HEIGHT / 2) + (wall_height / 2);

        for (int y = wall_ceiling; y < wall_floor; y++)
        {
            if (y < 0 || y >= SCREEN_HEIGHT)
                continue;
            int d = y - wall_ceiling;
            int tex_y = (d * 64) / wall_height;
            if (tex_y < 0)
                tex_y = 0;
            if (tex_y > 63)
                tex_y = 63;

            uint32_t color = tex_wall[tex_y * 64 + tex_x];
            if (is_dark)
                color = (color & 0xFEFEFE) >> 1;
            framebuffer[y * SCREEN_WIDTH + x] = color;
        }
    }

    for (int i = 1; i <= 4; i++)
    {
        if (i == p.id || Packets_players[i].x == 0 || !Packets_players[i].is_alive)
            continue;

        float sprite_x = Packets_players[i].x - p.x;
        float sprite_y = Packets_players[i].y - p.y;
        float sprite_angle = atan2f(sprite_y, sprite_x) - p.angle;

        while (sprite_angle < -3.14159f)
            sprite_angle += 2.0f * 3.14159f;
        while (sprite_angle > 3.14159f)
            sprite_angle -= 2.0f * 3.14159f;

        float sprite_dist = sqrtf(sprite_x * sprite_x + sprite_y * sprite_y);
        if (sprite_dist < 0.1f)
            sprite_dist = 0.1f;

        if (fabs(sprite_angle) < FOV)
        {
            int sprite_screen_x = (int)((SCREEN_WIDTH / 2) + (sprite_angle / FOV) * SCREEN_WIDTH);
            int sprite_size = (int)(SCREEN_HEIGHT / (sprite_dist * 1.5f));
            int sprite_ceil = SCREEN_HEIGHT / 2 - sprite_size / 2;
            int sprite_floor = SCREEN_HEIGHT / 2 + sprite_size / 2;
            int start_x = sprite_screen_x - sprite_size / 2;
            int end_x = sprite_screen_x + sprite_size / 2;

            for (int x = start_x; x < end_x; x++)
            {
                if (x < 0 || x >= SCREEN_WIDTH || sprite_dist >= z_buffer[x])
                    continue;
                int tex_x = ((x - start_x) * 64) / sprite_size;
                if (tex_x < 0)
                    tex_x = 0;
                if (tex_x > 63)
                    tex_x = 63;

                for (int y = sprite_ceil; y < sprite_floor; y++)
                {
                    if (y < 0 || y >= SCREEN_HEIGHT)
                        continue;
                    int tex_y = ((y - sprite_ceil) * 64) / sprite_size;
                    if (tex_y < 0)
                        tex_y = 0;
                    if (tex_y > 63)
                        tex_y = 63;

                    uint32_t color = tex_enemy[tex_y * 64 + tex_x];
                    if (color != 0)
                        framebuffer[y * SCREEN_WIDTH + x] = color;
                }
            }
        }
    }

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = SCREEN_WIDTH;
    bmi.bmiHeader.biHeight = -SCREEN_HEIGHT;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBitsToDevice(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, SCREEN_HEIGHT, framebuffer, &bmi, DIB_RGB_COLORS);

    // =========================================================
    // 5. NOUVELLE INTERFACE GDI (SANS LA CROIX MÉDICALE)
    // =========================================================

    // A. Le Viseur
    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2;
    HPEN crossPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
    HPEN oldPen = (HPEN)SelectObject(hdc, crossPen);
    MoveToEx(hdc, center_x - 12, center_y, NULL);
    LineTo(hdc, center_x - 4, center_y);
    MoveToEx(hdc, center_x + 4, center_y, NULL);
    LineTo(hdc, center_x + 12, center_y);
    MoveToEx(hdc, center_x, center_y - 12, NULL);
    LineTo(hdc, center_x, center_y - 4);
    MoveToEx(hdc, center_x, center_y + 4, NULL);
    LineTo(hdc, center_x, center_y + 12);
    SelectObject(hdc, oldPen);
    DeleteObject(crossPen);

    // B. Paramètres de la barre de vie
    int bar_x = 20; // Recalée sur la gauche
    int bar_y = SCREEN_HEIGHT - 45;
    int max_width = 200;
    int bar_height = 20;

    // C. Contour sombre de la barre
    HBRUSH borderBrush = CreateSolidBrush(RGB(15, 15, 15));
    RECT borderRect = {bar_x - 2, bar_y - 2, bar_x + max_width + 2, bar_y + bar_height + 2};
    FillRect(hdc, &borderRect, borderBrush);
    DeleteObject(borderBrush);

    // D. Fond de la barre
    HBRUSH bgBrush = CreateSolidBrush(RGB(60, 20, 20));
    RECT bgRect = {bar_x, bar_y, bar_x + max_width, bar_y + bar_height};
    FillRect(hdc, &bgRect, bgBrush);
    DeleteObject(bgBrush);

    // E. Remplissage coloré dynamique
    int current_width = (int)(((float)p.hp / 100.0f) * max_width);
    if (current_width < 0)
        current_width = 0;
    if (current_width > max_width)
        current_width = max_width;

    HBRUSH hpBrush;
    if (p.hp > 50)
        hpBrush = CreateSolidBrush(RGB(46, 204, 113));
    else if (p.hp > 25)
        hpBrush = CreateSolidBrush(RGB(241, 196, 15));
    else
        hpBrush = CreateSolidBrush(RGB(231, 76, 60));

    RECT hpRect = {bar_x, bar_y, bar_x + current_width, bar_y + bar_height};
    FillRect(hdc, &hpRect, hpBrush);
    DeleteObject(hpBrush);

    // F. Effet "Armure segmentée"
    HBRUSH segmentBrush = CreateSolidBrush(RGB(15, 15, 15));
    for (int i = 1; i < 10; i++)
    {
        int seg_x = bar_x + (i * (max_width / 10));
        RECT segRect = {seg_x, bar_y, seg_x + 2, bar_y + bar_height};
        FillRect(hdc, &segRect, segmentBrush);
    }
    DeleteObject(segmentBrush);

    // G. Texte des PV
    char hp_text[32];
    sprintf(hp_text, "%d / 100", p.hp);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, bar_x + max_width + 15, bar_y + 2, hp_text, strlen(hp_text));
}

void draw_tutorial(HDC hdc, DWORD tutorial_time)
{
    DWORD current_time = GetTickCount();
    DWORD elapsed = current_time - tutorial_time;

    // Afficher le tutoriel pendant 10 secondes seulement
    if (elapsed > 10000)
        return;

    // Paramètres du tutoriel
    int tutorial_x = SCREEN_WIDTH - 260;
    int tutorial_y = 15;
    int tutorial_width = 245;
    int tutorial_height = 165;

    // Couleur de fond avec alpha (semi-transparent)
    HBRUSH tutorial_bg = CreateSolidBrush(RGB(20, 20, 40));
    RECT tutorial_rect = {tutorial_x, tutorial_y, tutorial_x + tutorial_width, tutorial_y + tutorial_height};
    FillRect(hdc, &tutorial_rect, tutorial_bg);
    DeleteObject(tutorial_bg);

    // Bordure
    HPEN tutorial_border = CreatePen(PS_SOLID, 2, RGB(100, 150, 255));
    HPEN old_pen = (HPEN)SelectObject(hdc, tutorial_border);

    // Dessiner le rectangle
    MoveToEx(hdc, tutorial_x, tutorial_y, NULL);
    LineTo(hdc, tutorial_x + tutorial_width, tutorial_y);
    LineTo(hdc, tutorial_x + tutorial_width, tutorial_y + tutorial_height);
    LineTo(hdc, tutorial_x, tutorial_y + tutorial_height);
    LineTo(hdc, tutorial_x, tutorial_y);

    SelectObject(hdc, old_pen);
    DeleteObject(tutorial_border);

    // Texte du tutoriel
    SetTextColor(hdc, RGB(100, 150, 255));
    SetBkMode(hdc, TRANSPARENT);

    HFONT font = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
    HFONT old_font = (HFONT)SelectObject(hdc, font);

    // Titre
    TextOut(hdc, tutorial_x + 10, tutorial_y + 8, "-- CONTROLES --", 15);

    // Touches
    int line_y = tutorial_y + 30;
    TextOut(hdc, tutorial_x + 10, line_y, "Z/Haut : Avancer", 16);
    line_y += 16;

    TextOut(hdc, tutorial_x + 10, line_y, "S/Bas : Reculer", 15);
    line_y += 16;

    TextOut(hdc, tutorial_x + 10, line_y, "Q/Gauche : Gauche", 17);
    line_y += 16;

    TextOut(hdc, tutorial_x + 10, line_y, "D/Droite : Droite", 17);
    line_y += 16;

    TextOut(hdc, tutorial_x + 10, line_y, "SOURIS : Regarder", 16);
    line_y += 16;

    TextOut(hdc, tutorial_x + 10, line_y, "CLIC : Tirer", 12);

    SelectObject(hdc, old_font);
    DeleteObject(font);

    // Afficher "Cette fenetre disparait dans X sec"
    SetTextColor(hdc, RGB(150, 150, 150));
    HFONT small_font = CreateFont(10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
    HFONT old_small_font = (HFONT)SelectObject(hdc, small_font);

    int remaining_secs = 10 - (elapsed / 1000);
    char timer_text[32];
    sprintf(timer_text, "Disparait dans %d sec", remaining_secs);
    TextOut(hdc, tutorial_x + 10, tutorial_y + tutorial_height - 18, timer_text, strlen(timer_text));

    SelectObject(hdc, old_small_font);
    DeleteObject(small_font);
}