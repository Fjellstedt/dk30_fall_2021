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

struct key_state
{
    b32 isDown;
};

struct windows_input
{
    /* TODO(PF): Hour 3 Continue here.
    b32 WasDown(u8 key)
    {
        return keyStates[currentFrame][key];
    }
    b32 IsDown(u8 key)
    {
        return keyStates[currentFrame][key];
    }
    */
#define FRAMES 2
    key_state keyStates[FRAMES][256];
    u8 currentFrame = 0;
};

static void WinClearBackBuffer(window_back_buffer *buffer, u32 color)
{
    
#if 0
    // STUDY(pf) This should work ?: No, memset works on a byte basis, our color is 4 bytes wide.
    memset((u32 *)buffer->pixels, color, buffer->width * buffer->height);
#else
    u32 *onePastBufferPtr = (u32*)(buffer->pixels) + buffer->width * buffer->height;
    for(u32 *pixelPtr = (u32*)buffer->pixels;
        pixelPtr != onePastBufferPtr;
        pixelPtr++)
    {
        *pixelPtr = color;
    }
#endif
}

// TODO(pf): Relativity of some sort for locations. Currently just
// assumes that the locations are in buffer space.
static void WinRenderRectangle(window_back_buffer *buffer, u32 color, u32 x, u32 y,
                               u32 width, u32 height)
{
    // TODO(pf): Flip y.
    // NOTE(pf): clamp values.
    u32 minX = min(x, buffer->width);
    u32 minY = min(y, buffer->height);
    u32 maxX = min(x + width, buffer->width);
    u32 maxY = min(y + height, buffer->height);
    if(minX == maxX || minY == maxY)
        return;

    u32 *pixelPtr = (u32*)(buffer->pixels) + minX + buffer->width * minY;
    for(u32 yIteration = minY; yIteration < maxY; ++yIteration)
    {
        u32* xPixelPtr = pixelPtr;
        for(u32 xIteration = minX; xIteration < maxX; ++xIteration)
        {
            *xPixelPtr++ = color;
        }
        pixelPtr += buffer->width;
    }
}

static void WinPresentBackBuffer(window_back_buffer *backBuffer, HWND hwnd)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    u32 clientWidth = clientRect.right - clientRect.left;
    u32 clientHeight = clientRect.bottom - clientRect.top; // NOTE(pf): Y is flipped.

    HDC dc = GetDC(hwnd);
    // NOTE(pf): Display our back buffer to the window, i.e present.
    StretchDIBits(dc, 0, 0, clientWidth, clientHeight,
                  0, 0, backBuffer->width, backBuffer->height, backBuffer->pixels,
                  &backBuffer->info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }break;
        default:
        {
            // TODO(pf): Check if messages that we want to capture in
            // our mainloop gets passed in here.
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

    windows_input input = {};
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

        // NOTE(pf): Colors: 0xAARRGGBB.
        u32 clearColor = 0xFF0000FF;
        WinClearBackBuffer(&backBuffer, clearColor);

        // NOTE(pf): Bottom Left.
        u32 boxWidth = 50, boxHeight = 50;
        u32 rectColor = 0xFFFF0000;
        WinRenderRectangle(&backBuffer, rectColor, 0, 0,
                           boxWidth, boxHeight);
        // NOTE(pf): Bottom Right.
        rectColor = 0xFF00FF00;
        WinRenderRectangle(&backBuffer, rectColor, backBuffer.width - boxWidth, 0,
                           boxWidth, boxHeight);
        // NOTE(pf): Top Left.
        rectColor = 0xFFFFFF00;
        WinRenderRectangle(&backBuffer, rectColor, 0, backBuffer.height - boxHeight,
                           boxWidth, boxHeight);
        // NOTE(pf): Top Right.
        rectColor = 0xFFFFFFFF;
        WinRenderRectangle(&backBuffer, rectColor, backBuffer.width - boxWidth, backBuffer.height - boxHeight,
                           boxWidth, boxHeight);
        WinPresentBackBuffer(&backBuffer, hwnd);
        
    }
    
    return 0;
}
