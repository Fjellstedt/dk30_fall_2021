#if !defined(MAIN_H)
/* ========================================================================
   $Creator: Patrik Fjellstedt $
   ======================================================================== */


#include <Windows.h>
#include <assert.h>
#include <cmath>
#include <stdio.h>

#define KB(bits)(bits * 1024)
#define MB(bits)(KB(bits) * 1024)
#define GB(bits)(MB(bits) * 1024)

enum class tile_type
{
    TILE_INVALID,
    TILE_EMPTY,
    TILE_BLOCKING
};

struct area_tile
{
    tile_type type;
};

struct rect
{
    static rect CreateRect(s32 x, s32 y);
    
    b32 Contains(s32 x, s32 y);
    
    s32 minX;
    s32 minY;
    s32 maxX;
    s32 maxY;
};

struct area
{
    
    b32 isValid;
    u32 absMinX;
    u32 absMinY;
    u32 absMinZ;
    void *tiles;
};

struct world
{
/* TODO(pf): When generating an area:
   - Make all tiles on the outside into blocking.
   - Then when we generate a neighbouring area remove blocking tiles so that we
   can access that area from both areas in opposing ends.
   - Use a seed for generating the areas so they can be re-generated.
   TODO(pf): Think about how we can perm. store any changes.
*/
    void GenerateArea(area *result, struct game_state *gameState, s32 x, s32 y, s32 z);
    
    area *GetAreaBasedOnLocation(struct game_state *gameState, s32 tileX, s32 tileY, s32 tileZ);
    
#define AREA_COUNT 256
    area areas[AREA_COUNT];

    u32 TILE_WIDTH = 40;
    u32 TILE_HEIGHT = 40;
    u32 TILES_PER_WIDTH = 10; // NOTE(pf): MAX: 16,843,009
    u32 TILES_PER_HEIGHT = 10; // NOTE(pf): MAX: 16,843,009
};

struct camera
{
    // NOTE(pf): Center screen coordinates.
    s32 x; 
    s32 y;
};

struct entity
{
    f32 relX;
    f32 relY;
    s32 tileX;
    s32 tileY;
    s32 tileZ; // NOTE(pf): Increment when moving between layers?
    u32 color;
};

struct game_state
{
    template<typename T>
    T *Allocate(u32 amount);
        
    void *memory;
    u32 memorySize;
    u32 currentUsedBytes;
    world world;
    camera mainCamera;
    b32 toggleCameraSnapToPlayer;

    entity player;
};

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
    
    b32 IsPressed(u8 key)
    {
        return !keyStates[PreviousFrame()][key].isDown && keyStates[currentFrame][key].isDown;
    }
        
    b32 IsHeld(u8 key)
    {
        return keyStates[currentFrame][key].isDown;
    }


    s32 mouseX;
    s32 mouseY;
    s32 mouseZ;
    static constexpr u32 MAX_FRAME_CAPTURE = 2;
    // TODO(pf): Safer way to capture all keys ? Maybe map them into a
    // known set and ignore any keys outside it ?
    key_state keyStates [MAX_FRAME_CAPTURE][256]; 
    s8 currentFrame = 0;
};


#define MAIN_H
#endif
