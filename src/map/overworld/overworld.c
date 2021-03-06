#include "overworld.h"
#include <map/interface.h>
#define MAP_NAME overworld
#define NUM_ANIMATED_TILES 4
#define NUM_ANIMATED_FRAMES 4
#define NUM_MAPS 1
#define MAP16_Y 64
#define MAP16_X 128
#define MAP_ANIMATED overworld_animated
USING_MODULE(overworld_animated, PAGE_C);
extern const uint8_t overworld_animated[4][4][2][16];
#define MAP_MAP16 overworld_map0_map16
#define MAP0_MAP16 overworld_map0_map16
USING_MODULE(overworld_map0_map16, PAGE_D);
extern const uint8_t overworld_map0_map16[64][128];
#define MAP_TILES16 overworld_tiles16
USING_MODULE(overworld_tiles16, PAGE_C);
extern const uint8_t overworld_tiles16[256][2][2];
#define MAP_TILES8L overworld_tiles8L
USING_MODULE(overworld_tiles8L, PAGE_C);
extern const uint8_t overworld_tiles8L[256][2][16];
#define MAP_TILES8R overworld_tiles8R
USING_MODULE(overworld_tiles8R, PAGE_C);
extern const uint8_t overworld_tiles8R[256][2][16];
#include <map/implementation.h>
