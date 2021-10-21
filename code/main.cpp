/* ======================================================================== 
$Creator: Patrik Fjellstedt $
======================================================================== */ 

#include "pch.h"

#include <Windows.h>
#include <assert.h>
#include <cmath>
#include <stdio.h>

struct window_back_buffer
{
    u32 width;
    u32 height;
    u32 bytesPerPixel;
    u32 stride;
    void *pixels;
    BITMAPINFO info;
};

static u64 WinQueryPerformanceFrequency()
{
    LARGE_INTEGER largeInt;
    QueryPerformanceFrequency(&largeInt);
    return largeInt.QuadPart;
}


static u64 WinQueryPerformanceCounter()
{
    LARGE_INTEGER largeInt;
    QueryPerformanceCounter(&largeInt);
    return largeInt.QuadPart;
}

static f64 secondsPerCount = 1 / (f64)WinQueryPerformanceFrequency();
struct clock_cycles
{
    void Start()
    {
        startCycleCount = WinQueryPerformanceCounter();
    }
    
    void Stop()
    {
        startCycleCount = 0;
        endCycleCount = 0;
    }
    
    f32 Clock()
    {
        f32 result;
        endCycleCount = WinQueryPerformanceCounter();
        result = (f32)((f64)(endCycleCount - startCycleCount) * secondsPerCount);
        return result;
    }
    
    u64 startCycleCount;
    u64 endCycleCount;
};

struct key_state
{
    b32 isDown;
};

struct windows_input
{
    inline u32 PreviousFrame()
    {
        return (((currentFrame - 1) + MAX_FRAME_CAPTURE) % MAX_FRAME_CAPTURE);
    }
    
    void AdvanceFrame()
    {
        for(int i = 0; i < 256; ++i)
        {
            keyStates[PreviousFrame()][i].isDown = keyStates[currentFrame][i].isDown;
        }
        
        currentFrame = ++currentFrame % MAX_FRAME_CAPTURE;
    }
    
    b32 Pressed(u8 key)
    {
        return !keyStates[PreviousFrame()][key].isDown && keyStates[currentFrame][key].isDown;
    }
    
    b32 WasDown(u8 key)
    {
        return keyStates[PreviousFrame()][key].isDown && !keyStates[currentFrame][key].isDown;
    }
    
    b32 IsDown(u8 key)
    {
        return keyStates[currentFrame][key].isDown;
    }


    s32 mouseX;
    s32 mouseY;
    static constexpr u32 MAX_FRAME_CAPTURE = 2;
    // TODO(pf): Safer way to capture all keys ? Maybe map them into a
    // known set and ignore any keys outside it ?
    key_state keyStates [MAX_FRAME_CAPTURE][256]; 
    s8 currentFrame = 0;
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
    HDC dc = GetDC(hwnd);
    // NOTE(pf): Display our back buffer to the window, i.e present.
    StretchDIBits(dc, 0, 0, backBuffer->width, backBuffer->height,
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
            assert(true);
        }break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static void WinResizeBackBuffer(window_back_buffer *buffer, u32 width, u32 height)
{
    buffer->width = width;
    buffer->height = height;
    buffer->bytesPerPixel = 4;
    buffer->stride = buffer->width * buffer->bytesPerPixel;
    buffer->pixels = VirtualAlloc(0, buffer->width * buffer->height * buffer->bytesPerPixel,
                                    MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = width;
    buffer->info.bmiHeader.biHeight = height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = (WORD)buffer->bytesPerPixel * 8;
    buffer->info.bmiHeader.biCompression = BI_RGB;

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

    u32 windowWidth = 1920 / 2;
    u32 windowHeight = 1080 / 2;
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "DK30", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
                               0, 0, hInstance, 0);

    if(hwnd == 0)
    {
        return 0;
    }

    // TODO(pf): We shouldn't be working in absolute units anyways, but works for now.
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    u32 newWidth = windowWidth + (windowWidth - (clientRect.right - clientRect.left));
    u32 newHeight = windowHeight + (windowHeight - (clientRect.bottom - clientRect.top));
    SetWindowPos(hwnd, 0, CW_USEDEFAULT, CW_USEDEFAULT, newWidth, newHeight,
                 SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);

    ShowWindow(hwnd, nCmdShow);
    b32 isRunning = true;
    window_back_buffer backBuffer = {};
    WinResizeBackBuffer(&backBuffer, 1920/2, 1080/2);

    f32 targetFPS = 60;
    f32 targetFrameRate = 1.0f / targetFPS;
    f32 totTime = 0.0f;
    f32 dt = 0.0f;
    clock_cycles masterClock = {};
    f32 playerPosX = 0;
    f32 playerPosY = 0;
    u32 playerColor = 0xFFFFFFFF;
    
    windows_input input = {};
    HDC dc = GetDC(hwnd);
    // NOTE(pf): Main Loop.
    while(isRunning)
    {
        masterClock.Start();
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
                
                // NOTE(pf): Apparently the keyCode is 0 whenever
                // the key is released for mouse buttons.. TODO: STUDY why maybe ?
                case WM_LBUTTONUP:
                {
                    key_state *state = &input.keyStates[input.currentFrame][VK_LBUTTON];
                    state->isDown = false;
                }break;
                case WM_LBUTTONDOWN:
                {
                    key_state *state = &input.keyStates[input.currentFrame][VK_LBUTTON];
                    state->isDown = true;
                }break;
                case WM_RBUTTONUP:
                {
                    key_state *state = &input.keyStates[input.currentFrame][VK_RBUTTON];
                    state->isDown = false;
                }break;
                case WM_RBUTTONDOWN:
                {   
                    key_state *state = &input.keyStates[input.currentFrame][VK_RBUTTON];
                    state->isDown = true;
                }break;
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYUP:
                case WM_SYSKEYDOWN:
                {
                    u32 keyCode = (u32)msg.wParam;
                    b32 wasDown = (msg.lParam & (1 << 30)) != 0;
                    b32 isDown = (msg.lParam & (1 << 31)) == 0;
                    if(wasDown != isDown)
                    {
                        key_state *state = &input.keyStates[input.currentFrame][keyCode];
                        state->isDown = isDown;                   
#if 0
                        char dbgTxt[256];
                        _snprintf_s(dbgTxt, sizeof(dbgTxt), "Key: %d changed to: %d\n", keyCode, isDown ? 1 : 0);
                        OutputDebugStringA(dbgTxt);
#endif
                    }
                }break;
                default:
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }break;
            }
        }

        // NOTE(pf): "Everything Else Block"

        // NOTE(pf): Input
        
        POINT mouseP = {};
        GetCursorPos(&mouseP);
        ScreenToClient(hwnd, &mouseP);
        input.mouseX = mouseP.x;
        input.mouseY = mouseP.y;

        b32 shouldSnapBoxToCursor = false;
        if(input.IsDown(VK_LBUTTON))
        {
            shouldSnapBoxToCursor = true;
        }
        
        if(input.IsDown(VK_ESCAPE))
        {
            isRunning = false;
            continue;
        }
        
        f32 deltaX = 0.0f;
        f32 deltaY = 0.0f;
        if(input.IsDown('W'))
        {
            deltaY += 1.0f;
        }
        if(input.IsDown('S'))
        {
            deltaY -= 1.0f;
        }
        if(input.IsDown('A'))
        {
            deltaX -= 1.0f;
        }
        if(input.IsDown('D'))
        {
            deltaX += 1.0f;
        }
        
        playerPosX += deltaX;
        playerPosY += deltaY;
        f32 dtCos = abs(cos(totTime));
        playerColor = ((1 << 24) * (u32)(255.0f * dtCos) |
                       (1 << 16) * (u32)(255.0f * dtCos) |
                       (1 << 8) * (u32)(255.0f * dtCos) | (u32)(255.0f * dtCos));

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


        char dbgTxt[256];
        _snprintf_s(dbgTxt, sizeof(dbgTxt), "Mouse X: %d Mouse Y: %d\n", input.mouseX, input.mouseY);
        OutputDebugStringA(dbgTxt);
            
        if(shouldSnapBoxToCursor)
        {
            WinRenderRectangle(&backBuffer, playerColor, input.mouseX, input.mouseY, 100, 100);
        }
        else
        {
            WinRenderRectangle(&backBuffer, playerColor, (u32)playerPosX, (u32)playerPosY, 100, 100);
        }
        

        // NOTE(pf): Present our buffer.
        WinPresentBackBuffer(&backBuffer, hwnd);

        // NOTE(pf): End of frame processing.
        input.AdvanceFrame();
        
        // TODO(pf): Not spin locking. Also make 
        do
        {
            dt = masterClock.Clock(); // NOTE(pf): To MS.
        }while(dt < targetFrameRate);
#if 0
        char dbgTxt[256];
        _snprintf_s(dbgTxt, sizeof(dbgTxt), "dt: %f totTime: %f\n", dt, totTime);
        OutputDebugStringA(dbgTxt);
#endif
        dt = targetFrameRate;
        totTime += dt;
    }
    
    return 0;
}
