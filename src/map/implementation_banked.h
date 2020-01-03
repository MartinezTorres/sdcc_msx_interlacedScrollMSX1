#include <map/common.h>
#include <tms99X8.h>

#ifdef MSX
static void TMS99X8_memcpy8(uint16_t dst, const uint8_t *src) {

	register const uint8_t *p = src;
	VDP1 = dst & 0xFF; 
	VDP1 = 0x40 | (dst>>8);
	REPEAT8(VDP0 = *p++;);
}

static void TMS99X8_memcpy16(uint16_t dst, const uint8_t *src) {

	register const uint8_t *p = src;
	VDP1 = dst & 0xFF; 
	VDP1 = 0x40 | (dst>>8);
	REPEAT16(VDP0 = *p++;);
}

#else
static void TMS99X8_memcpy8 (uint16_t dst, const uint8_t *src) { TMS99X8_memcpy(dst,src,8); }
static void TMS99X8_memcpy16(uint16_t dst, const uint8_t *src) { TMS99X8_memcpy(dst,src,16); }
#endif

INLINE void freePattern(uint8_t bank, uint8_t pattern) { map.bank[bank].freePatterns[--map.bank[bank].numUsedPatterns] = pattern; }
INLINE uint8_t getPattern(uint8_t bank) { return map.bank[bank].freePatterns[map.bank[bank].numUsedPatterns++]; }

INLINE void renew_tile8_bank(uint8_t bank, uint8_t rowScreen8, uint8_t colScreen8) {
    
    uint8_t rowWorld8 = rowScreen8 + map.pos.i;
    uint8_t colWorld8 = colScreen8 + map.pos.j;
    
    uint8_t PN_Idx =  (rowWorld8<<5) + colWorld8;

    uint8_t rowWorld16 = rowWorld8 >> 1;
    uint8_t colWorld16 = colWorld8 >> 1;

    uint8_t oldTile8 = map.bank[bank].tile8[PN_Idx]; // up to 256 tile8s to fit in a segment.
    uint8_t stageSegment = map.stages[rowWorld16>>1][colWorld16>>1]; // stages apply in blocks of 32x32 pixels
    mapper_load_module(MAP_TILES16, PAGE_C);
    mapper_load_segment(stageSegment, PAGE_D);
    {
        uint8_t tile16Idx = MAP_MAP16[rowWorld16][colWorld16];
        uint8_t tile8Idx = MAP_TILES16[tile16Idx][rowWorld8&1][colWorld8&1];
        if (tile8Idx==oldTile8) return;
        printf("(%2d,%2d) PN_Idx:%3d count:%d\n",rowScreen8, colScreen8, PN_Idx,map.bank[bank].count[tile8Idx]);
        if (map.bank[bank].count[tile8Idx]++ == 0) {
            
            uint8_t patternIdx = map.bank[bank].pattern[tile8Idx] = getPattern(bank);
            mapper_load_module(MAP_TILES8, PAGE_C);
            TMS99X8_memcpy16(MODE2_ADDRESS_PG + bank*(8*256) + (((uint16_t)patternIdx)<<4), MAP_TILES8[tile8Idx][0]);
            TMS99X8_memcpy16(MODE2_ADDRESS_CT + bank*(8*256) + (((uint16_t)patternIdx)<<4), MAP_TILES8[tile8Idx][1]);
        }
        map.bank[bank].tile8[PN_Idx] = tile8Idx;
        map.bank[bank].PN[PN_Idx] = map.bank[bank].pattern[tile8Idx]<<1;
    }
    
    if (oldTile8 && --map.bank[bank].count[oldTile8]==0) {
        printf("freeing pattern\n");
        freePattern(bank, map.bank[bank].pattern[oldTile8]);
    }
}

void APPEND(MAP_NAME,_renew_tile8)(uint8_t rowScreen8, uint8_t colScreen8) {
    
    if (rowScreen8<8) { 
        renew_tile8_bank( 0, rowScreen8, colScreen8);
    } else if (rowScreen8<16) {
        renew_tile8_bank( 1, rowScreen8, colScreen8);
    } else {
        renew_tile8_bank( 2, rowScreen8, colScreen8);
    }
}

void APPEND(MAP_NAME,_renew_col)(uint8_t colScreen8) {
    
    uint8_t i=0;
    for (i=0; i<24; i++)
        APPEND(MAP_NAME,_renew_tile8)(i,colScreen8);
}


static uint8_t PN_Idx;
static uint8_t bank;
static MapBank *bankPtr;

// TODO: OPTIMIZE BY ALIGNING OLDTILES TO 256
static uint8_t oldTiles8[32];
static void populateOldTilesRow(uint8_t *src) __z88dk_fastcall {
    
    uint8_t *target = &oldTiles8[0];
    REPEAT32(*target++ = *src++;);
}


////////////////////////////////////////////////////////////////////////
// reserves a pattern and loads it
INLINE uint8_t reserveTilePattern(uint8_t tile8Idx) {
    
    const uint8_t * const *mapTile8Ptr = MAP_TILES8[tile8Idx][0];
    uint8_t patternIdx = getPattern(bank);
    map.bank[bank].pattern[tile8Idx] = patternIdx;
    mapper_load_module(MAP_TILES8, PAGE_C);
    TMS99X8_memcpy16(MODE2_ADDRESS_PG + bank*(8*256) + (((uint16_t)patternIdx)<<4), *mapTile8Ptr);
    TMS99X8_memcpy16(MODE2_ADDRESS_CT + bank*(8*256) + (((uint16_t)patternIdx)<<4), *(mapTile8Ptr+16));
    return patternIdx;
}
BANK_DECLARATION(uint8_t, reserveTilePattern, uint8_t)


/*
#define BANK_DECLARATION(ret_type, name, param_type)  \
static ret_type name ## 0(param_type param) __z88dk_fastcall { return name ## _bank( 0, param); } \
static ret_type name ## 1(param_type param) __z88dk_fastcall { return name ## _bank( 1, param); } \
static ret_type name ## 2(param_type param) __z88dk_fastcall { return name ## _bank( 2, param); } \
 \
INLINE ret_type name (uint8_t bank, param_type param) { \
    if (bank==0) return name ## 0(param); \
    if (bank==1) return name ## 1(param); \
    if (bank==2) return name ## 2(param); \
}



////////////////////////////////////////////////////////////////////////
// reserves a pattern and loads it

INLINE uint8_t reserveTilePattern_bank(uint8_t bank, uint8_t tile8Idx) {
    
    const uint8_t * const *mapTile8Ptr = MAP_TILES8[tile8Idx][0];
    uint8_t patternIdx = getPattern(bank);
    map.bank[bank].pattern[tile8Idx] = patternIdx;
    mapper_load_module(MAP_TILES8, PAGE_C);
    TMS99X8_memcpy16(MODE2_ADDRESS_PG + bank*(8*256) + (((uint16_t)patternIdx)<<4), *mapTile8Ptr);
    TMS99X8_memcpy16(MODE2_ADDRESS_CT + bank*(8*256) + (((uint16_t)patternIdx)<<4), *(mapTile8Ptr+16));
    return patternIdx;
}
BANK_DECLARATION(uint8_t, reserveTilePattern, uint8_t)

////////////////////////////////////////////////////////////////////////
// gets the pattern or the current asked tile, reserves and loads one if the pattern is not loaded

inline static void getTilePattern_placeholder() {
    
    __asm
_getTilePattern0:
    ld  h, #>(_map) 
	ld	a, (hl)
	or	a, a
    jp  Z,_reserveTilePattern0
	inc	a
	ld	(hl), a
    inc h
	ld	l, (hl)
	ret

_getTilePattern1:
    ld  h, #>(_map+1280) 
	ld	a, (hl)
	or	a, a
    jp  Z,_reserveTilePattern1
	inc	a
	ld	(hl), a
    inc h
	ld	l, (hl)
	ret

_getTilePattern2:
    ld  h, #>(_map+2*1280) 
	ld	a, (hl)
	or	a, a
    jp  Z,_reserveTilePattern2
	inc	a
	ld	(hl), a
    inc h
	ld	l, (hl)
	ret
    __endasm;
}
static uint8_t getTilePattern0(uint8_t) __z88dk_fastcall;
static uint8_t getTilePattern1(uint8_t) __z88dk_fastcall;
static uint8_t getTilePattern2(uint8_t) __z88dk_fastcall;
INLINE uint8_t getTilePattern(uint8_t bank, uint8_t param) {
    if (bank==0) return getTilePattern0(param);
    if (bank==1) return getTilePattern1(param);
    if (bank==2) return getTilePattern2(param);
}
/*
static void populate_tile8_row0(uint16_t tile8IdxAndPNIdx) __z88dk_fastcall {
    uint8_t tile8Idx = tile8IdxAndPNIdx>>8;
    uint8_t PN_Idx = tile8IdxAndPNIdx&0xFF;
    
    uint8_t pattern = getTilePattern(0,tile8Idx);
    uint8_t *p = &map.bank[0].tile8[PN_Idx];
    *p = tile8Idx;
    map.bank[0].tile8[PN_Idx] = tile8Idx;
    map.bank[0].PN[PN_Idx] = pattern;

    return PN_Idx+1;    
}*/

static uint8_t PN_Idx;
static uint8_t bank;
static MapBank *bankPtr;
static void populate_tile8_rowP(uint8_t *tile8IdxPtr) __z88dk_fastcall {
    uint8_t tile8Idx = *tile8IdxPtr;
    bankPtr->tile8[PN_Idx] = tile8Idx;
    bankPtr->PN[PN_Idx] = getTilePattern(bank,tile8Idx)<<1;
    PN_Idx++;    
}
/*
INLINE void renew_row_bank(uint8_t bank, uint8_t rowScreen8) {
    // We must ensure that no region 33x9 has 256 of the same block type
    
    uint8_t rowWorld8 = rowScreen8 + map.pos.i;
    uint8_t colWorld8 = map.pos.j;

    uint8_t rowWorld16 = rowWorld8 >> 1;
    uint8_t colWorld16 = colWorld8 >> 1;
    
    uint8_t PN_Idx =  (rowWorld8<<5) + colWorld8;
    uint8_t *stagePtr = &map.stages[rowWorld16>>1][colWorld16>>1];

    populateOldTilesRow(&map.bank[bank].tile8[PN_Idx]);
    
    mapper_load_module(MAP_TILES16, PAGE_C);
    {    
        const uint8_t *tile16IdxPtr = &MAP_MAP16[rowWorld16][colWorld16];
        {
            mapper_load_segment(*stagePtr, PAGE_D);
            {
                const uint8_t *tile8IdxPtr = &MAP_TILES16[*tile16IdxPtr][rowWorld8&1][0];
                if (colWorld8&1 == 0) {
                    uint8_t tile8Idx = *tile8IdxPtr;
                    map.bank[bank].tile8[PN_Idx] = tile8Idx;
                    map.bank[bank].PN[PN_Idx] = getTilePattern(bank,tile8Idx)<<1;
                    PN_Idx++;
                }
                tile8IdxPtr++;
                {
                    uint8_t tile8Idx = *tile8IdxPtr;
                    map.bank[bank].tile8[PN_Idx] = tile8Idx;
                    map.bank[bank].PN[PN_Idx] = getTilePattern(bank,tile8Idx)<<1;
                    PN_Idx++;
                }
                stagePtr += ((uint8_t)tile16IdxPtr) & 1;
                tile16IdxPtr++;
            }
        }
        
        {
            uint8_t j;
            for (j=0; j<15; j++) {
                mapper_load_segment(*stagePtr, PAGE_D);
                {
                    const uint8_t *tile8IdxPtr = &MAP_TILES16[*tile16IdxPtr][rowWorld8&1][0];
                    {
                        uint8_t tile8Idx = *tile8IdxPtr;
                        map.bank[bank].tile8[PN_Idx] = tile8Idx;
                        map.bank[bank].PN[PN_Idx] = getTilePattern(bank,tile8Idx)<<1;
                        PN_Idx++;
                    }
                    tile8IdxPtr++;
                    {
                        uint8_t tile8Idx = *tile8IdxPtr;
                        map.bank[bank].tile8[PN_Idx] = tile8Idx;
                        map.bank[bank].PN[PN_Idx] = getTilePattern(bank,tile8Idx)<<1;
                        PN_Idx++;
                    }
                    stagePtr += ((uint8_t)tile16IdxPtr) & 1;
                    tile16IdxPtr++;                    
                }
            }
        }

        if (colWorld8&1 == 1) {
            mapper_load_segment(*stagePtr, PAGE_D);
            {
                const uint8_t *tile8IdxPtr = &MAP_TILES16[*tile16IdxPtr][rowWorld8&1][0];
                {
                    uint8_t tile8Idx = *tile8IdxPtr;
                    map.bank[bank].tile8[PN_Idx] = tile8Idx;
                    map.bank[bank].PN[PN_Idx] = getTilePattern(bank,tile8Idx)<<1;
                    PN_Idx++;
                }
            }
        }
    }
}

static void renew_row0(uint8_t rowScreen8) __z88dk_fastcall { renew_row_bank( 0, rowScreen8); }

static void renew_row(uint8_t rowScreen8) {
    if (rowScreen8<8) { 
        renew_row_bank( 0, rowScreen8);
    } else if (rowScreen8<16) {
        renew_row_bank( 1, rowScreen8);
    } else {
        renew_row_bank( 2, rowScreen8);
    }
}*/

void APPEND(MAP_NAME,_renew_row)(uint8_t rowScreen8) {
    
    uint8_t j=0;
    for (j=0; j<32; j++)
        APPEND(MAP_NAME,_renew_tile8)(rowScreen8,j);
}

void APPEND(MAP_NAME,_init)() {

    TMS99X8_memset(MODE2_ADDRESS_PG, 0, 3*8*256);
    TMS99X8_memset(MODE2_ADDRESS_CT, 0, 3*8*256);
    TMS99X8_memset(MODE2_ADDRESS_PN0, 0, 3*256);
    TMS99X8_memset(MODE2_ADDRESS_PN1, 0, 3*256);
    
    memset(&map,0,sizeof(map));
    {
        uint8_t b,i;
        for (b=0; b<3; b++) {

            for (i=0; i<128-16; i++) 
                map.bank[b].freePatterns[i]=i+16;

            for (i=0; i<NUM_ANIMATED_TILES; i++) 
                map.bank[b].count[i]++;

            for (i=0; i<NUM_ANIMATED_TILES; i++) 
                map.bank[b].pattern[i] = i+1;

            mapper_load_module(MAP_ANIMATED, PAGE_C);
            for (i=0; i<NUM_ANIMATED_TILES; i++) {
                TMS99X8_memcpy16(MODE2_ADDRESS_PG + b*(8*256) + (((uint16_t)i+1)<<4), MAP_ANIMATED[i][0][0]);
                TMS99X8_memcpy16(MODE2_ADDRESS_CT + b*(8*256) + (((uint16_t)i+1)<<4), MAP_ANIMATED[i][0][1]);
            }
        }
    }
    
    memset(&map.stages[0], MODULE_SEGMENT(MAP0_MAP16, PAGE_D), sizeof(map.stages));
}



INLINE void update_animation_buffer0(uint8_t animatedTile, uint8_t animatedFrame) {

    TMS99X8_memcpy8(MODE2_ADDRESS_PG + 0*(8*256) + (((uint16_t)animatedTile+1)<<4) + 0, &MAP_ANIMATED[animatedTile][animatedFrame][0][0]);
    TMS99X8_memcpy8(MODE2_ADDRESS_CT + 0*(8*256) + (((uint16_t)animatedTile+1)<<4) + 0, &MAP_ANIMATED[animatedTile][animatedFrame][1][0]);
    TMS99X8_memcpy8(MODE2_ADDRESS_PG + 1*(8*256) + (((uint16_t)animatedTile+1)<<4) + 0, &MAP_ANIMATED[animatedTile][animatedFrame][0][0]);
    TMS99X8_memcpy8(MODE2_ADDRESS_CT + 1*(8*256) + (((uint16_t)animatedTile+1)<<4) + 0, &MAP_ANIMATED[animatedTile][animatedFrame][1][0]);
    TMS99X8_memcpy8(MODE2_ADDRESS_PG + 2*(8*256) + (((uint16_t)animatedTile+1)<<4) + 0, &MAP_ANIMATED[animatedTile][animatedFrame][0][0]);
    TMS99X8_memcpy8(MODE2_ADDRESS_CT + 2*(8*256) + (((uint16_t)animatedTile+1)<<4) + 0, &MAP_ANIMATED[animatedTile][animatedFrame][1][0]);
}

INLINE void update_animation_buffer1(uint8_t animatedTile, uint8_t animatedFrame) {

    TMS99X8_memcpy8(MODE2_ADDRESS_PG + 0*(8*256) + (((uint16_t)animatedTile+1)<<4) + 8, &MAP_ANIMATED[animatedTile][animatedFrame][0][8]);
    TMS99X8_memcpy8(MODE2_ADDRESS_CT + 0*(8*256) + (((uint16_t)animatedTile+1)<<4) + 8, &MAP_ANIMATED[animatedTile][animatedFrame][1][8]);
    TMS99X8_memcpy8(MODE2_ADDRESS_PG + 1*(8*256) + (((uint16_t)animatedTile+1)<<4) + 8, &MAP_ANIMATED[animatedTile][animatedFrame][0][8]);
    TMS99X8_memcpy8(MODE2_ADDRESS_CT + 1*(8*256) + (((uint16_t)animatedTile+1)<<4) + 8, &MAP_ANIMATED[animatedTile][animatedFrame][1][8]);
    TMS99X8_memcpy8(MODE2_ADDRESS_PG + 2*(8*256) + (((uint16_t)animatedTile+1)<<4) + 8, &MAP_ANIMATED[animatedTile][animatedFrame][0][8]);
    TMS99X8_memcpy8(MODE2_ADDRESS_CT + 2*(8*256) + (((uint16_t)animatedTile+1)<<4) + 8, &MAP_ANIMATED[animatedTile][animatedFrame][1][8]);
}

void APPEND(MAP_NAME,_update_animation)(EM2_Buffer em2_Buffer) {
    
    uint8_t i;
    mapper_load_module(MAP_ANIMATED, PAGE_C);
    if (em2_Buffer==0) {
        for (i=0; i<NUM_ANIMATED_TILES; i++) update_animation_buffer0(i,map.animatedFrame);
    } else {
        for (i=0; i<NUM_ANIMATED_TILES; i++) update_animation_buffer1(i,map.animatedFrame);
    }
}

void APPEND(MAP_NAME,_iterate_animation)() {
    
    uint8_t i = map.animatedFrame;
    i++;
    if (i>=NUM_ANIMATED_FRAMES) i=0;
    map.animatedFrame = i;
}

