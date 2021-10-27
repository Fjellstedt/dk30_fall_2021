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
    struct entity *entity; // TODO(pf): REMOVE, just for testing.
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
    area_tile *GetTile(struct world * world, s32 x, s32 y);
    b32 isValid;
    s8 x;
    s8 y;
    s16 z;
    void *tiles;
    entity *activeEntities;
    // entity *firstStaticEntity; TODO(pf): Maybe split entities like this ?
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
    void GenerateArea(area *result, struct game_state *gameState, s8 x, s8 y, s16 z);
    
    area *GetArea(struct game_state *gameState, s8 areaX, s8 areaY, s16 areaZ);

#define AREA_X_MAX 255
#define AREA_Y_MAX 255
#define AREA_Z_MAX 255 * 255
#define AREA_COUNT 256
    area areas[AREA_COUNT];

    const s32 TILE_WIDTH = 40;
    const s32 TILE_HEIGHT = 40;
    const s32 TILES_PER_WIDTH = 10;
    const s32 TILES_PER_HEIGHT = 10;
    // NOTE(pf): For centering.
    const s32 HALF_TILE_WIDTH = TILE_WIDTH / 2;
    const s32 HALF_TILE_HEIGHT = TILE_HEIGHT / 2;
    const s32 HALF_TILES_PER_WIDTH = TILES_PER_WIDTH / 2;
    const s32 HALF_TILES_PER_HEIGHT = TILES_PER_HEIGHT / 2;
};

struct camera
{
    // NOTE(pf): Center screen coordinates.
    s32 x; 
    s32 y;
    s32 z;
    
    s8 areaX;
    s8 areaY;
    s16 areaZ;

    s32 dbgZoom;
};

// NOTE(pf): Entity tile/area coordinates are larger than they have to
// be if we need to conserve memory footprint later.. if we make them
// compact make sure we sync with how area indexing works.
struct entity
{
    // NOTE(pf): Positioning relative code BEGIN (move out ?)
    // NOTE(pf): Local to tile.
    f32 relX;
    f32 relY;
    // TODO(pf): Want a rel Z for jumping or something ?
    
    // NOTE(pf): Local to area.
    s32 tileX;
    s32 tileY;
    
    // NOTE(pf): Think of as 'global' area positiong, if these wrap we come back to previous area.
    s8 areaX;
    s8 areaY;
    s16 areaZ;
    // NOTE(pf): Positioning relative code END.

    // NOTE(pf): Gameplay relative code BEGIN
    s32 health;
    s32 maxHealth;
    u32 entityArrayIndex; // TODO(pf): REMOVE, only for testing.
    u32 color;
    // NOTE(pf): Used in structures that daisy chain entities.
    entity *nextEntity; 
    // NOTE(pf): Gameplay relative code END
};

struct game_state
{
    // TODO(pf)BLOCK:BEGIN Move out into some memory management unit ?
    template<typename T>
    T *Allocate(u32 amount);
        
    void *memory;
    u32 memorySize;
    u32 currentUsedBytes;
    // TODO(pf)BLOCK: END

    entity *AllocateGameEntityAndAddToArea(s8 areaX, s8 areaY, s16 areaZ);
    void ReturnGameEntityToPoolAndRemoveFromArea(entity* entity);
    
    world world;
    camera mainCamera;
    b32 toggleCameraSnapToPlayer;

#define MAX_ENTITY_COUNT 256
    u32 activeEntityCount;
    entity entities[MAX_ENTITY_COUNT];
    entity *firstFreeEntity;
    u32 playerControlIndex;
    //entity player;
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
