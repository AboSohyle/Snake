#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#ifndef UNICODE
#define UNICODE 1
#endif

#ifndef _UNICODE
#define _UNICODE 1
#endif

#define _WIN32_WINNT _WIN32_WINNT_WIN10
#define _WIN32_IE _WIN32_IE_IE100

#include <Windows.h>
#include "Snake.h"
#include "resource.h"

Snake *game;
int cx = 816;
int cy = 614;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        game = new Snake();
        if (!game->Initialize(hWnd))
        {
            MessageBox(hWnd, L"Creating game class failed.", L"Error", MB_ICONERROR);
            PostQuitMessage(0);
        }
        break;
    case WM_SIZE:
        game->Resize(LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_KEYDOWN:
        game->Play(wParam);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    int ret = -1;

    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
    {
        WNDCLASSEX wc = {};

        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = &WndProc;
        wc.hInstance = hInstance;
        wc.hCursor = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
        wc.hbrBackground = CreateSolidBrush(RGB(31, 31, 31));
        wc.lpszClassName = L"Snake";
        wc.hIcon = (HICON)LoadImage(hInstance,
                                    MAKEINTRESOURCE(IDI_APPICON),
                                    IMAGE_ICON, 0, 0,
                                    LR_DEFAULTSIZE | LR_DEFAULTCOLOR | LR_SHARED);
        wc.hIconSm = (HICON)LoadImage(hInstance,
                                      MAKEINTRESOURCE(IDI_APPICON),
                                      IMAGE_ICON,
                                      GetSystemMetrics(SM_CXSMICON),
                                      GetSystemMetrics(SM_CYSMICON),
                                      LR_DEFAULTCOLOR | LR_SHARED);

        if (!RegisterClassEx(&wc))
        {
            MessageBox(NULL, L"Registering main class failed.", L"Error", MB_ICONERROR);
            CoUninitialize();
            return ret;
        }

        RECT rect;
        GetClientRect(GetDesktopWindow(), &rect);
        rect.left = (rect.right / 2) - (cx / 2);
        rect.top = (rect.bottom / 2) - (cy / 2);

        HWND hWnd = CreateWindowEx(0, wc.lpszClassName, wc.lpszClassName,
                                   WS_OVERLAPPEDWINDOW,
                                   rect.left, rect.top,
                                   cx, cy,
                                   NULL, NULL, hInstance, NULL);

        if (!hWnd)
        {
            MessageBox(NULL, L"Creating main window failed.", L"Error", MB_ICONERROR);
            CoUninitialize();
            return ret;
        }

        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        MSG msg;

        while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0)
        {
            if (ret == -1)
                break;
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        ret = (int)msg.wParam;
        CoUninitialize();
    }

    return ret;
}
