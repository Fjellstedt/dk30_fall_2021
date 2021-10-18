/* ======================================================================== 
$Creator: Patrik Fjellstedt $
======================================================================== */ 

#include "pch.h"
#include <Windows.h>

struct window_back_buffer
{
    u32 width;
    u32 height;
    u32 bytesPerPixel;
    u32 stride;
    void *pixels;
    BITMAPINFO info;
};

static void WinClearBackBuffer(window_back_buffer *buffer, u32 color)
{
    // NOTE(pf) This should work ? memset((u32 *)buffer->pixels, color, buffer->stride * buffer->height);
    u32 *onePastBufferPtr = (u32*)(((u8*)buffer->pixels) + buffer->stride * (buffer->height));
    for(u32 *pixelPtr = (u32*)buffer->pixels;
        pixelPtr != onePastBufferPtr;
        pixelPtr++)
    {
        *pixelPtr = color;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // NOTE(pf): Setup.
    const char CLASS_NAME[] = "DK30";

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    
    RegisterClass(&wc);
        
    
    u32 windowWidth = 1920/2;
    u32 windowHeight = 1080/2;
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "DK30", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
                               0, 0, hInstance, 0);

    if(hwnd == 0)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    
    b32 isRunning = true;
    window_back_buffer backBuffer = {};
    backBuffer.width = windowWidth;
    backBuffer.height = windowHeight;
    backBuffer.bytesPerPixel = 4;
    backBuffer.stride = backBuffer.width * backBuffer.bytesPerPixel;
    backBuffer.pixels = VirtualAlloc(0, backBuffer.width * backBuffer.height * backBuffer.bytesPerPixel,
                                    MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    backBuffer.info.bmiHeader.biSize = sizeof(backBuffer.info.bmiHeader);
    backBuffer.info.bmiHeader.biWidth = windowWidth;
    backBuffer.info.bmiHeader.biHeight = windowHeight;
    backBuffer.info.bmiHeader.biPlanes = 1;
    backBuffer.info.bmiHeader.biBitCount = (WORD)backBuffer.bytesPerPixel * 8;
    backBuffer.info.bmiHeader.biCompression = BI_RGB;

    HDC dc = GetDC(hwnd);
    // NOTE(pf): Main Loop.
    while(isRunning)
    {
        // NOTE(pf): Msg pump.
        MSG msg = {};
        while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            switch(msg.message)
            {
                case WM_DESTROY:
                {
                    PostQuitMessage(0);
                }break;
                default:
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }break;
            }
        }

        // NOTE(pf): Rendering
        
        u32 color = 0xFFFF00FF;
        WinClearBackBuffer(&backBuffer, color);
            
        // NOTE(pf): Display our back buffer to the window, i.e present.
        StretchDIBits(dc, 0, 0, windowWidth, windowHeight,
                      0, 0, windowWidth, windowHeight, backBuffer.pixels,
                      &backBuffer.info, DIB_RGB_COLORS, SRCCOPY);
    }
    
    return 0;
}
