/* ======================================================================== 
$Creator: Patrik Fjellstedt $
======================================================================== */ 

#include "pch.h"
#include "main.h"

rect rect::CreateRect(s32 x, s32 y)
{
    rect result = {};
    return result;
}
    
b32 rect::Contains(s32 x, s32 y)
{
    b32 result = false; // TODO(pf)
    return result;
}

b32 area::IsValid()
{
     // NOTE(pf): All non zero, then valid, otherwise the 'non-valid' area.
    return relX || relY || relZ;
}

area *world::GenerateArea(game_state *gameState, s32 x, s32 y, s32 z)
{
    area *result = gameState->Allocate<area>(1);
    result->relX = x;
    result->relY = y;
    result->relZ = z;
    u32 tileCount = TILES_PER_WIDTH * TILES_PER_HEIGHT;
    result->tiles = gameState->Allocate<tile>(tileCount);
    for(tile *currentTile = (tile *)(result->tiles);
        currentTile != (tile *)(result->tiles) + tileCount;
        ++currentTile)
    {
        currentTile->type = tile_type::TILE_EMPTY;
    }
    return result;
}

area *world::GetAreaBasedOnLocation(game_state *gameState, s32 tileX, s32 tileY, s32 tileZ)
{
    area *result = 0;
    u32 indexX = tileX / TILES_PER_WIDTH;
    u32 indexY = tileY / TILES_PER_HEIGHT;
    u32 indexZ = tileZ;
    // STUDY(pf): How can we generate a lookup index that doesn't overlap with other tile locations ?
    u32 index = indexX << 13 | indexY << 7 | indexZ;
    result = &areas[index];
    if(!result->IsValid())
    {
        result = GenerateArea(gameState, indexX, indexY, indexZ);
    }
    return result;
}

template<typename T>
T *game_state::Allocate(u32 amount)
{
    u32 allocationSizeInBytes = sizeof(T) * amount;
    assert((currentUsedBytes + allocationSizeInBytes) < memorySize); // NOTE(pf): Out of memory!
    T *result = (T *)((u8*)memory + currentUsedBytes);
    currentUsedBytes += allocationSizeInBytes;
    return result;
}

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
static void WinRenderRectangle(window_back_buffer *buffer, u32 color, s32 x, s32 y,
                               s32 width, s32 height)
{
    assert(width >= 0 && height >= 0); // NOTE(pf): avoid unsigned promotion in min/max  but still want to make sure the user doesnt mess up.
    // TODO(pf): Flip y.
    // NOTE(pf): clamp values.
    s32 minX = max(x, 0);
    s32 minY = max(y, 0);
    s32 maxX = min(x + width, (s32)buffer->width);
    s32 maxY = min(y + height, (s32)buffer->height);
    if(minX == maxX || minY == maxY)
        return;

    u32 *pixelPtr = (u32*)(buffer->pixels) + minX + buffer->width * minY;
    for(s32 yIteration = minY; yIteration < maxY; ++yIteration)
    {
        u32* xPixelPtr = pixelPtr;
        for(s32 xIteration = minX; xIteration < maxX; ++xIteration)
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

    game_state gameState = {};
    gameState.memorySize = MB(24);
    gameState.memory = VirtualAlloc(0, gameState.memorySize,
                                    MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
    //gameState.world.tileCount = 10;
    gameState.world.tiles = gameState.Allocate<tile>(10);
    gameState.mainCamera.x = 0;
    gameState.mainCamera.y = 0;
    
    b32 isRunning = true;
    window_back_buffer backBuffer = {};
    WinResizeBackBuffer(&backBuffer, windowWidth, windowHeight);

    f32 targetFPS = 60;
    f32 targetFrameRate = 1.0f / targetFPS;
    f32 totTime = 0.0f;
    f32 dt = 0.0f;
    clock_cycles masterClock = {};
    entity *player = &gameState.player;
    player->relX = windowWidth / 2.0f;  
    player->relY = windowHeight / 2.0f;
    player->color = 0xFFFFFFFF;
    
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
        input.mouseY = windowHeight - mouseP.y;
        
        if(input.IsPressed(VK_ESCAPE))
        {
            isRunning = false;
            continue;
        }

        // TODO(pf): Proper mafths (vectors).
        f32 speed = 5.0f;
        f32 playerDeltaX = 0.0f;
        f32 playerDeltaY = 0.0f;
        f32 cameraDeltaX = 0.0f;
        f32 cameraDeltaY = 0.0f;
        
        if(input.IsHeld('W'))
        {
            playerDeltaY += 1.0f;
        }
        if(input.IsHeld('S'))
        {
            playerDeltaY -= 1.0f;
        }
        if(input.IsHeld('A'))
        {
            playerDeltaX -= 1.0f;
        }
        if(input.IsHeld('D'))
        {
            playerDeltaX += 1.0f;
        }
        if(input.IsHeld(VK_UP))
        {
            cameraDeltaY += 1.0f;
        }
        if(input.IsHeld(VK_DOWN))
        {
            cameraDeltaY -= 1.0f;
        }
        if(input.IsHeld(VK_LEFT))
        {
            cameraDeltaX -= 1.0f;
        }
        if(input.IsHeld(VK_RIGHT))
        {
            cameraDeltaX += 1.0f;
        }
        
        if(input.IsHeld(VK_SHIFT))
        {
            speed = 50.0f;
        }
        if(input.IsPressed(VK_SPACE))
        {
            gameState.toggleCameraSnapToPlayer = !gameState.toggleCameraSnapToPlayer;
        }

        player->relX += playerDeltaX * speed;
        player->relY += playerDeltaY * speed;
        player->tileX = (s32)(player->relX / gameState.world.TILE_WIDTH);
        player->tileY = (s32)(player->relY / gameState.world.TILE_HEIGHT);
        
        gameState.mainCamera.x += (s32)(cameraDeltaX * speed);
        gameState.mainCamera.y += (s32)(cameraDeltaY * speed);
       
        if(gameState.toggleCameraSnapToPlayer)
        {
            gameState.mainCamera.x = (s32)(player->relX - windowWidth / 2);
            gameState.mainCamera.y = (s32)(player->relY - windowHeight / 2);
        }
        else
        {
            gameState.mainCamera.x += (s32)(cameraDeltaX * speed);
            gameState.mainCamera.y += (s32)(cameraDeltaY * speed);    
        }
        
        f32 dtCos = abs(cos(totTime));
        player->color = ((1 << 24) * (u32)(255.0f * dtCos) |
                         (1 << 16) * (u32)(255.0f * dtCos) |
                         (1 << 8) * (u32)(255.0f * dtCos) | (u32)(255.0f * dtCos));


        
        // NOTE(pf): Colors: 0xAARRGGBB.
        u32 clearColor = 0xFF0000FF;
        WinClearBackBuffer(&backBuffer, clearColor);

        // TODO(pf): separate rendering from logic, just hacking atm.
        world *world = &gameState.world;
        u32 tileStride = 10 / 2;
        // NOTE(pf): Very hacky placement of tiles atm.
        // TODO(pf): Logic for 'rooms' and relative tile positioning.
        // TODO(pf): Move away from using pixel measurements.
        for(u32 y = 0; y < tileStride; ++y)
        {
            for(u32 x = 0; x < tileStride; ++x)
            {
                tile *activeTile = world->tiles + x + (y * tileStride);
                s32 absTileX = (s32)((x - (tileStride/2)) * world->TILE_WIDTH);
                s32 absTileY = (s32)((y - (tileStride/2))* world->TILE_HEIGHT);
                // NOTE(pf): "Convert to camera space"
                s32 tileX = absTileX - gameState.mainCamera.x;
                s32 tileY = absTileY - gameState.mainCamera.y;                
                // NOTE(pf): Check if we are inside.
                if(input.IsPressed(VK_LBUTTON))
                {
                    if(input.mouseX >= tileX &&
                       input.mouseX < (s32)(tileX + world->TILE_WIDTH) &&
                       input.mouseY >= tileY &&
                       input.mouseY < (s32)(tileY + world->TILE_HEIGHT))
                    {
                        if(activeTile->type == tile_type::TILE_EMPTY)
                        {
                            activeTile->type = tile_type::TILE_BLOCKING;
                        }
                        else if (activeTile->type == tile_type::TILE_BLOCKING)
                        {
                            activeTile->type = tile_type::TILE_BLOCKING;
                        }
                    }
                }

                u32  tileColor = 0xFFFF00FF;
                switch(activeTile->type)
                {
                    case tile_type::TILE_EMPTY:
                    {
                        tileColor = ((1 << 24) * ((u32)(x) % 128) |
                                     (1 << 16) * ((u32)(y) % 128) |
                                     (1 << 8) * ((u32)(x) % 128) |
                                     (1 << 0) * ((u32)(y) % 128));
                    }break;
                    case tile_type::TILE_BLOCKING:
                    {
                        tileColor = 0xAAAAAAAA;
                    }break;
                    default:
                        assert(false);
                }
                
                WinRenderRectangle(&backBuffer, tileColor,
                                   tileX + 2, tileY + 2, world->TILE_WIDTH - 2, world->TILE_HEIGHT - 2);
            }
        }
        
#if 0
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
#endif
        WinRenderRectangle(&backBuffer, player->color,
                           (player->tileX * world->TILE_WIDTH) - gameState.mainCamera.x,
                           (player->tileY * world->TILE_WIDTH) - gameState.mainCamera.y,
                           world->TILE_WIDTH, world->TILE_HEIGHT);
        
        WinRenderRectangle(&backBuffer, 0x11111111, (s32)player->relX - 5  - gameState.mainCamera.x,
                           (s32)player->relY - 5 - gameState.mainCamera.y,
                           10, 10);
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
