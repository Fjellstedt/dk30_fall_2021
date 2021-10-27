/* ======================================================================== 
$Creator: Patrik Fjellstedt $
======================================================================== */ 

#include "pch.h"
#include "main.h"
#include <stdlib.h>

/* TODO(pf): General TODOS:
 * separate rendering from logic, just hacking atm.
 * Move away from using pixel measurements.
*/

// NOTE(pf): Lower and Upper bound inclusive.
// TODO(pf): STUDY(pf): How can we pass a seed to this function to force a certain randomness ?
s32 GenerateRandomValue(s32 lower, s32 upper)
{
    assert(lower < upper);
    return (rand() % (upper - lower)) + lower;
}

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

area_tile *area::GetTile(world *world, s32 tileX, s32 tileY)
{
    return (area_tile *)tiles + tileX + (tileY * world->TILES_PER_WIDTH);
}

entity *game_state::AllocateGameEntityAndAddToArea(s8 areaX, s8 areaY, s16 areaZ)
{
    assert(activeEntityCount + 1 < MAX_ENTITY_COUNT);
    entity *result = &entities[++activeEntityCount];
    result->relX = world.TILE_WIDTH / 2.0f;
    result->relY = world.TILE_HEIGHT / 2.0f;
    result->tileX = world.TILES_PER_WIDTH / 2;
    result->tileY = world.TILES_PER_HEIGHT / 2;
    result->color = ((255 << 24) |
                        (GenerateRandomValue(0, 255) << 16) |
                        (GenerateRandomValue(0, 255) << 8) |
                        (GenerateRandomValue(0, 255)));
    result->areaX = areaX;
    result->areaY = areaY;
    result->areaZ = areaZ;
    result->entityArrayIndex = activeEntityCount;
    result->health = 10;
    result->maxHealth = 10;

    // TODO(pf): Passing self? Probably should take a look at the architecture.
    area *area = world.GetArea(this, areaX, areaY, areaZ);
    if(area->activeEntities)
    {
        result->nextEntity = area->activeEntities;
    }
    area->activeEntities = result;
    area->GetTile(&world, result->tileX, result->tileY)->entity = result;
    return result;
}

void game_state::ReturnGameEntityToPoolAndRemoveFromArea(struct entity* entity)
{
    if(firstFreeEntity)
    {
        entity->nextEntity = firstFreeEntity;
    }
    firstFreeEntity = entity;
    firstFreeEntity->entityArrayIndex = 0;
    // TODO(pf): Passing self? Probably should take a look at the architecture.
    area *activeArea = world.GetArea(this, firstFreeEntity->areaX, firstFreeEntity->areaY, firstFreeEntity->areaZ);
    struct entity **areaEntity = &activeArea->activeEntities;
    for(;
        areaEntity != nullptr;
        ++areaEntity)
    {
        if(*areaEntity == firstFreeEntity)
        {
            *areaEntity = (*areaEntity)->nextEntity;
            break;
        }
    }
    // NOTE(pf): Make sure the entity was removed, currently we assume an entity will never be removed if it wasn't in an area.
    assert((areaEntity == nullptr && activeArea->activeEntities == nullptr) || areaEntity != nullptr);
    --activeEntityCount;
    assert(activeEntityCount > 0 && activeEntityCount < MAX_ENTITY_COUNT);
}

void world::GenerateArea(area *result, game_state *gameState, s8 x, s8 y, s16 z)
{
    result->x = x;
    result->y = y;
    result->z = z;
    
    u32 tileCount = TILES_PER_WIDTH * TILES_PER_HEIGHT;
    result->tiles = gameState->Allocate<area_tile>(tileCount);
    for(area_tile *currentTile = (area_tile *)(result->tiles);
        currentTile != (area_tile *)(result->tiles) + tileCount;
        ++currentTile)
    {
        currentTile->type = tile_type::TILE_EMPTY;
    }
    
    // TODO(pf): ... make more intresting random generation.
    for(s32 yBoarder = 0; yBoarder < TILES_PER_HEIGHT; ++yBoarder)
    {
        for(s32 xBoarder = 0; xBoarder < TILES_PER_WIDTH; ++xBoarder)
        {
            if(yBoarder == 0 || yBoarder == (TILES_PER_HEIGHT - 1) ||
               xBoarder == 0 || xBoarder == (TILES_PER_WIDTH - 1))
            {
                area_tile *tile = (area_tile *)result->tiles + xBoarder + (yBoarder * TILES_PER_HEIGHT);
                tile->type = tile_type::TILE_BLOCKING;
                if(yBoarder == 0 && xBoarder == (TILES_PER_WIDTH / 2))
                {
                    tile->type = tile_type::TILE_EMPTY;
                }
                else if(yBoarder == (TILES_PER_HEIGHT - 1) && xBoarder == (TILES_PER_WIDTH / 2))
                {
                    tile->type = tile_type::TILE_EMPTY;
                }
                else if(xBoarder == 0 && yBoarder == (TILES_PER_HEIGHT / 2))
                {
                    tile->type = tile_type::TILE_EMPTY;
                }
                else if(xBoarder == (TILES_PER_WIDTH - 1) && yBoarder == (TILES_PER_HEIGHT / 2))
                {
                    tile->type = tile_type::TILE_EMPTY;
                }
            }
        }
    }
    result->isValid = true;

    gameState->AllocateGameEntityAndAddToArea(x, y, z);
}

area *world::GetArea(game_state *gameState, s8 areaX, s8 areaY, s16 areaZ)
{
    area *result = 0;
    // NOTE(pf): We have 8 bits for x and y and 16 for z.
    u32 index = areaX << 24 | areaY << 16 | areaZ << 0;
    int hashMapIndex = index % AREA_COUNT;
    result = &areas[hashMapIndex];
    if(!result->isValid)
    {
        GenerateArea(result, gameState, areaX, areaY, areaZ);
    }
    else if(result->x != areaX || result->y != areaY || result->z != areaZ)
    {
        // NOTE(pf): Index collision ? Linear search. Assert that we didn't wrap.
        int newHashIndex = hashMapIndex;
        while(++newHashIndex != hashMapIndex)
        {
            result = &areas[newHashIndex];
            if(!result->isValid)
            {
                GenerateArea(result, gameState, areaX, areaY, areaZ);
                break;
            }
            else if(result->x == areaX && result->y == areaY && result->z == areaZ)
            {
                break;
            }
        }
        assert(newHashIndex != hashMapIndex);
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

#if 0 // NOTE(pf): Old render routine.
// TODO(pf): Relativity of some sort for locations. Currently just
// assumes that the locations are in buffer space.
static void WinRenderRectangle_(window_back_buffer *buffer, u32 color, s32 x, s32 y,
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
#endif
static void WinRenderRectangleFromMidPoint(window_back_buffer *buffer, u32 color,
                                           s32 midX, s32 midY, s32 halfWidth, s32 halfHeight)
{
    assert(halfWidth >= 0 && halfHeight >= 0); // NOTE(pf): avoid unsigned promotion in min/max  but still want to make sure the user doesnt mess up.
    // TODO(pf): Flip y.
    // NOTE(pf): clamp values.
    s32 minX = max(midX - halfWidth, 0);
    s32 minY = max(midY - halfHeight, 0);
    s32 maxX = min(midX + halfWidth, (s32)buffer->width);
    s32 maxY = min(midY + halfHeight, (s32)buffer->height);
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
    u32 halfWindowWidth = windowWidth / 2;
    u32 halfWindowHeight = windowHeight / 2;
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

    gameState.mainCamera.x = 0;
    gameState.mainCamera.y = 0;
    world *world = &gameState.world;
    b32 isRunning = true;
    window_back_buffer backBuffer = {};
    WinResizeBackBuffer(&backBuffer, windowWidth, windowHeight);

    f32 targetFPS = 60;
    f32 targetFrameRate = 1.0f / targetFPS;
    f32 totTime = 0.0f;
    f32 dt = 0.0f;
    clock_cycles masterClock = {};
    
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
                case WM_MOUSEWHEEL:
                {
                    input.mouseZ += GET_WHEEL_DELTA_WPARAM(msg.wParam) / 120;
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
        f32 inputSpeed = 5.0f;
        f32 inputDeltaX = 0.0f;
        f32 inputDeltaY = 0.0f;
        f32 cameraDeltaX = 0.0f;
        f32 cameraDeltaY = 0.0f;
        
        if(input.IsHeld('W'))
        {
            inputDeltaY += 1.0f;
        }
        if(input.IsHeld('S'))
        {
            inputDeltaY -= 1.0f;
        }
        if(input.IsHeld('A'))
        {
            inputDeltaX -= 1.0f;
        }
        if(input.IsHeld('D'))
        {
            inputDeltaX += 1.0f;
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
            inputSpeed = 50.0f;
        }
        if(input.IsPressed(VK_SPACE))
        {
            gameState.toggleCameraSnapToPlayer = !gameState.toggleCameraSnapToPlayer;
        }

        // NOTE(pf): Spawn a unit.
        if(input.IsPressed('M'))
        {
            gameState.AllocateGameEntityAndAddToArea(gameState.mainCamera.areaX,
                                                     gameState.mainCamera.areaY,
                                                     gameState.mainCamera.areaZ);
        }

        gameState.mainCamera.dbgZoom = input.mouseZ;

        for(u32 entityIndex = 0;
            entityIndex < gameState.activeEntityCount;
            ++entityIndex)
        {
            entity *entity = &gameState.entities[entityIndex];
            
        }

        // NOTE(pf): One frame delayed now.
        if(!gameState.toggleCameraSnapToPlayer)
        {
            gameState.mainCamera.x += (s32)(cameraDeltaX * inputSpeed);
            gameState.mainCamera.y += (s32)(cameraDeltaY * inputSpeed);
        }

        gameState.mainCamera.areaX = (s8)(gameState.mainCamera.x / (world->HALF_TILES_PER_WIDTH * world->TILE_WIDTH));
        gameState.mainCamera.areaY = (s8)(gameState.mainCamera.y / (world->HALF_TILES_PER_HEIGHT * world->TILE_HEIGHT));
        gameState.mainCamera.areaZ = (s16)gameState.mainCamera.z;
        
        // NOTE(pf): Colors: 0xAARRGGBB.
        u32 clearColor = 0xFF0000FF;
        WinClearBackBuffer(&backBuffer, clearColor);
        
#if 1
        // NOTE(pf): fetch a grid of areas around the mainCamera location.
        s8 gridSize = 0;
        for(s8 gridY = gameState.mainCamera.areaY - gridSize;
            gridY <= gameState.mainCamera.areaY + gridSize;
            ++gridY)
        {
            for(s8 gridX = gameState.mainCamera.areaX - gridSize;
                gridX <= gameState.mainCamera.areaX + gridSize;
                ++gridX)
            {
                area *activeArea = world->GetArea(&gameState,
                                                  gameState.mainCamera.areaX + gridX,
                                                  gameState.mainCamera.areaY + gridY,
                                                  gameState.mainCamera.areaZ);
                
                assert(activeArea->isValid);
                // TODO(pf): Move away from tiles and only handle entities instead ?
                for(s32 y = 0; y < world->TILES_PER_HEIGHT; ++y)
                {
                    for(s32 x = 0; x < world->TILES_PER_WIDTH; ++x)
                    {
                        area_tile *activeTile = activeArea->GetTile(world, x, y);
                        s32 absTileX = (s32)((x + (activeArea->x * world->HALF_TILES_PER_WIDTH)) * world->TILE_WIDTH);
                        s32 absTileY = (s32)((y + (activeArea->y * world->HALF_TILES_PER_HEIGHT)) * world->TILE_HEIGHT);
                        // NOTE(pf): "Convert to camera space"
                        s32 tileX = absTileX + gameState.mainCamera.x;
                        s32 tileY = absTileY + gameState.mainCamera.y;
                        // NOTE(pf): Check if we are inside.
                        if(input.IsPressed(VK_LBUTTON))
                        {
                            if(input.mouseX >= (s32)(tileX) &&
                               input.mouseX < (s32)(tileX + world->HALF_TILE_WIDTH) &&
                               input.mouseY >= (s32)(tileY) &&
                               input.mouseY < (s32)(tileY + world->HALF_TILE_HEIGHT))
                            {
                                if(activeTile->type == tile_type::TILE_EMPTY)
                                {
                                    activeTile->type = tile_type::TILE_BLOCKING;
                                }
                                else if (activeTile->type == tile_type::TILE_BLOCKING)
                                {
                                    if(activeTile->entity)
                                    {
                                        gameState.playerControlIndex = activeTile->entity->entityArrayIndex;
                                    }
                                    else
                                    {
                                        activeTile->type = tile_type::TILE_EMPTY;
                                    }
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

                        WinRenderRectangleFromMidPoint(&backBuffer, tileColor,
                                           tileX, tileY,
                                           world->HALF_TILE_WIDTH - 1,
                                           world->HALF_TILE_HEIGHT - 1);
                    }
                }

                // NOTE(pf): Entity sim/rendering.
                for(entity *entity = activeArea->activeEntities;
                    entity != nullptr;
                    entity = entity->nextEntity)
                {

                    // NOTE(pf): Entity Sim:
                    // NOTE(pf): Split up projected location and do a bounds check
                    // with tiles to see if we can move there.
                
                    // TODO(pf): Very simplistic collision loop, good enough for
                    // now but it would be nice if the units could slide against
                    // the collider. Need vector concept for the dot product to
                    // project the units velocity. Lets do this when we introduce
                    // more entities, introduce a small math library then aswell.

                    // TODO(pf): Entity AI.
                    f32 entityDeltaX = 0.0f;
                    f32 entityDeltaY = 0.0f;
                    f32 entitySpeed = 0.0f;
                    if(entity->entityArrayIndex == gameState.playerControlIndex)
                    {
                        entityDeltaX = inputDeltaX;
                        entityDeltaY = inputDeltaY;
                        entitySpeed = inputSpeed;
                    }
            
                    f32 nextEntityRelX = entity->relX + entityDeltaX * entitySpeed;
                    f32 nextEntityRelY = entity->relY + entityDeltaY * entitySpeed;
                    s32 nextEntityTileX = entity->tileX;
                    s32 nextEntityTileY = entity->tileY;
                    s8 nextEntityAreaX = entity->areaX;
                    s8 nextEntityAreaY = entity->areaY;
                    s16 nextEntityAreaZ = entity->areaZ;
        
                    // NOTE(pf): Wrapping for relative pos within a tile..
                    if(nextEntityRelX < 0)
                    {
                        nextEntityRelX += world->TILE_WIDTH;
                        --nextEntityTileX;
                    }
                    else if(nextEntityRelX >= world->TILE_WIDTH)
                    {
                        nextEntityRelX -= world->TILE_WIDTH;
                        ++nextEntityTileX;
                    }
            
                    if(nextEntityRelY < 0)
                    {
                        nextEntityRelY += world->TILE_HEIGHT;
                        --nextEntityTileY;
                    }
                    else if(nextEntityRelY >= world->TILE_WIDTH)
                    {
                        nextEntityRelY -= world->TILE_HEIGHT;
                        ++nextEntityTileY;
                    }
            
                    // .. wrapping for tile within an area
                    if(nextEntityTileX < 0)
                    {
                        nextEntityTileX += world->HALF_TILES_PER_WIDTH;
                        --nextEntityAreaX;
                    }
                    else if(nextEntityTileX >= world->HALF_TILES_PER_WIDTH)
                    {
                        nextEntityTileX -= world->HALF_TILES_PER_WIDTH;
                        ++nextEntityAreaX;
                    }
                    if(nextEntityTileY < 0)
                    {
                        nextEntityTileY += world->HALF_TILES_PER_HEIGHT;
                        --nextEntityAreaY;
                    }
                    else if(nextEntityTileY >= world->HALF_TILES_PER_HEIGHT)
                    {
                        nextEntityTileY -= world->HALF_TILES_PER_HEIGHT;
                        ++nextEntityAreaY;
                    }
        
                    b32 entityAllowedToMakeMovement = true;
                    if(entity->tileX != nextEntityTileX || entity->tileY != nextEntityTileY)
                    {
                        area * area = world->GetArea(&gameState,
                                                     nextEntityAreaX, nextEntityAreaY, nextEntityAreaZ);
                        area_tile *tile = area->GetTile(world, nextEntityTileX, nextEntityTileY);
                        if(tile->type == tile_type::TILE_BLOCKING)
                        {
                            entityAllowedToMakeMovement = false;
                            if(tile->entity)
                            {
                                tile->entity->health = max(tile->entity->health - 1, 0);
                                if(tile->entity->health <= 0)
                                {
                                    gameState.ReturnGameEntityToPoolAndRemoveFromArea(tile->entity);
                                    tile->entity = nullptr;
                                    tile->type = tile_type::TILE_EMPTY;
                                }
                            }
                        }
                    }

                    if(entityAllowedToMakeMovement)
                    {
                        entity->relX = nextEntityRelX;
                        entity->relY = nextEntityRelY;
                        area *area = world->GetArea(&gameState, entity->areaX, entity->areaY, entity->areaZ);
                        area_tile *prevTile = area->GetTile(world, entity->tileX, entity->tileY);
                        prevTile->type = tile_type::TILE_EMPTY;
                        prevTile->entity = nullptr;
                        entity->tileX = nextEntityTileX;
                        entity->tileY = nextEntityTileY;
                        entity->areaX = nextEntityAreaX;
                        entity->areaY = nextEntityAreaY;
                        // NOTE(pf): Need to refetch area, we could have moved!
                        area = world->GetArea(&gameState, entity->areaX, entity->areaY, entity->areaZ);
                        area_tile *newTile = area->GetTile(world, entity->tileX, entity->tileY);
                        newTile->entity = entity;
                        newTile->type = tile_type::TILE_BLOCKING;
                    }
       
                    if(gameState.playerControlIndex &&
                       gameState.toggleCameraSnapToPlayer &&
                       entity->entityArrayIndex == gameState.playerControlIndex)
                    {
                        gameState.mainCamera.x = (s32)
                            (((entity->areaX * world->TILES_PER_WIDTH) + entity->tileX)  * world->TILE_WIDTH +
                             entity->relX - halfWindowWidth);
                        gameState.mainCamera.y = (s32)
                            (((entity->areaY * world->TILES_PER_HEIGHT) + entity->tileY) * world->TILE_HEIGHT +
                             entity->relY - halfWindowHeight);
                    }

                    // NOTE(pf): Entity Rendering:
                    s32 entityAbsPosX = entity->areaX * world->HALF_TILES_PER_WIDTH * world->TILE_WIDTH;
                    s32 entityAbsPosY = entity->areaY * world->HALF_TILES_PER_HEIGHT * world->TILE_HEIGHT;
                    s32 tileRenderPosX = entityAbsPosX + (entity->tileX * world->TILE_WIDTH) + gameState.mainCamera.x;
                    s32 tileRenderPosY = entityAbsPosY + (entity->tileY * world->TILE_HEIGHT) + gameState.mainCamera.y;
                    // NOTE(pf): 'Entity Body'
                    WinRenderRectangleFromMidPoint(&backBuffer, entity->color,
                                       tileRenderPosX, tileRenderPosY,
                                       world->HALF_TILE_WIDTH, world->HALF_TILE_HEIGHT);
                    // NOTE(pf): 'Entity Rel Coordinate'
                    s32 relTileRenderPosX = entityAbsPosX + (s32)((entity->tileX * world->TILE_WIDTH) +
                                                                  entity->relX - world->HALF_TILE_WIDTH + gameState.mainCamera.x);
                    s32 relTileRenderPosY = entityAbsPosY + (s32)((entity->tileY * world->TILE_HEIGHT) +
                                                                  entity->relY - world->HALF_TILE_HEIGHT + gameState.mainCamera.y);
                    WinRenderRectangleFromMidPoint(&backBuffer, 0x11111111,
                                       relTileRenderPosX, relTileRenderPosY,
                                       2, 2);

                    // NOTE(pf): 'Entity Health Backdrop'
                    s32 halfHealthBarWidth = 20;
                    WinRenderRectangleFromMidPoint(&backBuffer, 0x00000000,
                                       tileRenderPosX, tileRenderPosY + world->HALF_TILE_HEIGHT,
                                       halfHealthBarWidth, 5);
                    // NOTE(pf): Calculate current health and render ontop as a red bar.
                    s32 MARGIN = 1;
                    s32 currentHealthBarWidth = (s32)(((f32)entity->health / (f32)entity->maxHealth) * halfHealthBarWidth);
                    WinRenderRectangleFromMidPoint(&backBuffer, 0x00FF0000,
                                       tileRenderPosX, tileRenderPosY + world->HALF_TILE_HEIGHT,
                                       currentHealthBarWidth - MARGIN, 5 - MARGIN);
                }
            }    
        }
#else         
        entity *activePlayer = &gameState.entities[gameState.playerControlIndex];
        area *playerActiveArea = world->GetArea(&gameState, activePlayer->areaX,
                                                activePlayer->areaY, activePlayer->areaZ);
        assert(playerActiveArea->isValid);
        
        // NOTE(pf): Tile Simulation.. TODO(pf): Remove and only have Entities, i.e Walls are Entities ?
        for(s32 y = 0; y < world->TILES_PER_HEIGHT; ++y)
        {
            for(s32 x = 0; x < world->TILES_PER_WIDTH; ++x)
            {
                area_tile *activeTile = playerActiveArea->GetTile(world, x, y);
                s32 absTileX = (s32)((x + (playerActiveArea->x * world->TILES_PER_WIDTH)) * world->TILE_WIDTH);
                s32 absTileY = (s32)((y + (playerActiveArea->y * world->TILES_PER_HEIGHT)) * world->TILE_HEIGHT);
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
                            if(activeTile->entity)
                            {
                                gameState.playerControlIndex = activeTile->entity->entityArrayIndex;
                            }
                            else
                            {
                                activeTile->type = tile_type::TILE_EMPTY;
                            }
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
                                   tileX + 2, tileY + 2,
                                   world->TILE_WIDTH - 2,
                                   world->TILE_HEIGHT - 2);
            }
        }
        
        for(u32 entityIndex = 0;
            entityIndex < gameState.activeEntityCount;
            ++entityIndex)
        {
            entity *entity = &gameState.entities[entityIndex];
            if(playerActiveArea->x == entity->areaX &&
               playerActiveArea->y == entity->areaY &&
               playerActiveArea->z == entity->areaZ)
            {
                s32 entityAbsPosX = entity->areaX * world->TILES_PER_WIDTH * world->TILE_WIDTH;
                s32 entityAbsPosY = entity->areaY * world->TILES_PER_HEIGHT * world->TILE_HEIGHT;
                s32 tileRenderPosX = entityAbsPosX + (entity->tileX * world->TILE_WIDTH) - gameState.mainCamera.x;
                s32 tileRenderPosY = entityAbsPosY + (entity->tileY * world->TILE_HEIGHT) - gameState.mainCamera.y;
                // NOTE(pf): 'Entity Body'
                WinRenderRectangle(&backBuffer, entity->color,
                                   tileRenderPosX, tileRenderPosY,
                                   world->TILE_WIDTH, world->TILE_HEIGHT);
                // NOTE(pf): 'Entity Rel Coordinate'
                s32 relTileRenderPosX = entityAbsPosX + (s32)((entity->tileX * world->TILE_WIDTH) + entity->relX - 5  - gameState.mainCamera.x);
                s32 relTileRenderPosY = entityAbsPosY + (s32)((entity->tileY * world->TILE_HEIGHT) + entity->relY - 5 - gameState.mainCamera.y);
                WinRenderRectangle(&backBuffer, 0x11111111,
                                   relTileRenderPosX, relTileRenderPosY,
                                   10, 10);

                // NOTE(pf): 'Entity Health Backdrop'
                s32 healthBarWidth = 40;
                WinRenderRectangle(&backBuffer, 0x00000000,
                                   tileRenderPosX, tileRenderPosY + world->TILE_HEIGHT,
                                   healthBarWidth, 10);
                // NOTE(pf): Calculate current health and render ontop as a red bar.
                s32 MARGIN = 2;
                s32 currentHealthBarWidth = (s32)(((f32)entity->health / (f32)entity->maxHealth) * healthBarWidth);
                WinRenderRectangle(&backBuffer, 0x00FF0000,
                                   tileRenderPosX + MARGIN, tileRenderPosY + MARGIN + world->TILE_HEIGHT,
                                   currentHealthBarWidth - MARGIN, 10 - MARGIN);
            }
        }
        
#endif

        // NOTE(pf): Camera Center
        WinRenderRectangleFromMidPoint(&backBuffer, 0x00FF00FF,
                                       gameState.mainCamera.x + halfWindowWidth, gameState.mainCamera.y  + halfWindowHeight,
                                       2, 2);
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
