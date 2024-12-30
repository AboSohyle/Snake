#pragma once

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define _WIN32_WINNT _WIN32_WINNT_WIN10

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <vector>

template <class Interface>
inline void SafeRelease(Interface **ppInterface)
{
    if (*ppInterface != NULL)
    {
        (*ppInterface)->Release();

        (*ppInterface) = NULL;
    }
};

class Snake
{
private:
    ID2D1Factory *factory;
    ID2D1HwndRenderTarget *render;
    IDWriteFactory *writefct;
    IDWriteTextFormat *foodfmt;
    ID2D1SolidColorBrush *brush;

    HWND window;
    BOOL grid, losser, mute;
    INT score, hscore, speed, row, col;
    POINTF food;
    WCHAR foods[11][sizeof("🥕") + 1];
    std::vector<POINTF> body;

    enum direction
    {
        left,
        up,
        right,
        down
    } direction;

    enum status
    {
        idle,
        pause,
        play
    } status;

    void Reset();
    void Food();
    void Move();
    void Draw();
    static LRESULT CALLBACK GameProc(HWND, UINT, WPARAM, LPARAM);

public:
    Snake();
    ~Snake();
    BOOL Initialize(HWND);
    BOOL Initialize(HWND, INT, INT, INT, INT);
    void Play(WPARAM);
    void Resize(INT cx, INT cy);
    int GetSpeed() const;
    int GetScore() const;
    int GetHighScore() const;
};
