#include <common.h>

////////////////////////////////////////////////////////////////////////
// AUDIO

/*static void audioUpdate(void) {
    
    uint8_t oldSegmentPageB = mapper_load_module(psg, PAGE_B);

    TMS99X8_setRegister(7,BWhite);
    PSG_init();    

    TMS99X8_setRegister(7,BLightRed);
    ayr_spin();

    TMS99X8_setRegister(7,BLightGreen);
    ayFX_spin();

    TMS99X8_setRegister(7,BLightBlue);    
    PSG_syncRegisters();
    
    TMS99X8_setRegister(7,BWhite);

    mapper_load_segment(oldSegmentPageB, PAGE_B);    

    TMS99X8_setRegister(7,BBlack);    
}*/


////////////////////////////////////////////////////////////////////////
// MAP

#include <map/overworld/overworld.h>
#include <map/common.h>
USING_MODULE(overworld, PAGE_B);

static uint8_tp targetPos;
static EM2_Buffer em2_Buffer;
static bool enableAnimations;
static uint8_t nAnimationCount; 
static uint8_t nAnimationUpdates;  

#ifdef MSX
static void PN_Copy0(uint8_t *p) __z88dk_fastcall;
static void PN_Copy1(uint8_t *p) __z88dk_fastcall;

inline static void PN_Copy_Placeholder() {

    __asm
_PN_Copy0:
	ld	c, l
	ld	b, h
	ld	e, #0x20
00001$:
	ld	a, e
	dec	e
	or	a, a
	ret	Z
	ld	a, (bc)
	out	(_VDP0), a
	inc	c
	ld	a, (bc)
	out	(_VDP0), a
	inc	c
	ld	a, (bc)
	out	(_VDP0), a
	inc	c
	ld	a, (bc)
	out	(_VDP0), a
	inc	c
	ld	a, (bc)
	out	(_VDP0), a
	inc	c
	ld	a, (bc)
	out	(_VDP0), a
	inc	c
	ld	a, (bc)
	out	(_VDP0), a
	inc	c
	ld	a, (bc)
	out	(_VDP0), a
	inc	c
	jr	00001$

_PN_Copy1:
	ld	c, l
	ld	b, h
	ld	e, #0x20
00002$:
	ld	a, e
	dec	e
	or	a, a
	ret	Z
	ld	a, (bc)
	inc	c
	inc	a
	out	(_VDP0), a
	ld	a, (bc)
	inc	c
	inc	a
	out	(_VDP0), a
	ld	a, (bc)
	inc	c
	inc	a
	out	(_VDP0), a
	ld	a, (bc)
	inc	c
	inc	a
	out	(_VDP0), a
	ld	a, (bc)
	inc	c
	inc	a
	out	(_VDP0), a
	ld	a, (bc)
	inc	c
	inc	a
	out	(_VDP0), a
	ld	a, (bc)
	inc	c
	inc	a
	out	(_VDP0), a
	ld	a, (bc)
	inc	c
	inc	a
	out	(_VDP0), a
	jr	00002$
	__endasm;
}

static void overworld_copy_PN0() {

	VDP1 = (0?MODE2_ADDRESS_PN1:MODE2_ADDRESS_PN0) & 0xFF;
    VDP1 = 0x40 | ( (0?MODE2_ADDRESS_PN1:MODE2_ADDRESS_PN0)>>8);
    {
        uint8_t PN_Start = (map.pos.i<<5) + map.pos.j;
        PN_Copy0(&map.bank[0].PN[PN_Start]);
        PN_Copy0(&map.bank[1].PN[PN_Start]);
        PN_Copy0(&map.bank[2].PN[PN_Start]);
    }
}

static void overworld_copy_PN1() {

	VDP1 = (1?MODE2_ADDRESS_PN1:MODE2_ADDRESS_PN0) & 0xFF;
    VDP1 = 0x40 | ( (1?MODE2_ADDRESS_PN1:MODE2_ADDRESS_PN0)>>8);
    {
        uint8_t PN_Start = (map.pos.i<<5) + map.pos.j;
        PN_Copy1(&map.bank[0].PN[PN_Start]);
        PN_Copy1(&map.bank[1].PN[PN_Start]);
        PN_Copy1(&map.bank[2].PN[PN_Start]);
    }
}

INLINE void overworld_copy_PN(EM2_Buffer buffer) { if (buffer==0) overworld_copy_PN0(); else overworld_copy_PN1(); }

#else
INLINE void overworld_copy_PN(EM2_Buffer buffer) {

    uint8_t bank;
    uint16_t vdp = buffer?MODE2_ADDRESS_PN1:MODE2_ADDRESS_PN0;
    for (bank=0; bank<3; bank++) {
        uint8_t PN_Start = (map.pos.i<<5) + map.pos.j;
        register uint8_t *p;
        register uint8_t iterations;
        
        if (PN_Start==0) {
            p = &map.bank[bank].PN[0];
            iterations = 256/4;
            while (iterations--) REPEAT4( TMS99X8_memset(vdp++,*p++ + buffer,1); );
        } else {
        
            p = &map.bank[bank].PN[PN_Start];
            iterations = 256-PN_Start;
            while (iterations--) TMS99X8_memset(vdp++,*p++ + buffer,1);
            
            p = &map.bank[bank].PN[0];
            iterations = PN_Start;
            while (iterations--) TMS99X8_memset(vdp++,*p++ + buffer,1);
        }
    }        
}
#endif

static enum { ISR_LEVEL_A, ISR_LEVEL_B } isr_level_state_int;
static enum { ISR_LEVEL_STANDBY, ISR_LEVEL_SUBMITTED} isr_level_state;

static uint8_t nDeployISR;

static void overworld_isr(void) {

    uint8_t oldSegmentPageB = mapper_load_module(overworld, PAGE_B);


    TMS99X8_activateBuffer(em2_Buffer);
    em2_Buffer = !em2_Buffer;
    
    if (nDeployISR) {
        
        overworld_copy_PN(em2_Buffer);
        nDeployISR--;
        
        if (nDeployISR == 1) {
            overworld_free();
            isr_level_state = ISR_LEVEL_STANDBY;
        }
    }

    if (nDeployISR==0)  {      
        
        if (isr_level_state == ISR_LEVEL_STANDBY) {
        
            nAnimationCount++;
            if (nAnimationCount>=12) {
                overworld_iterate_animation();
                nAnimationUpdates = 2;
                nAnimationCount=0;
            }
            
            if (enableAnimations && nAnimationUpdates) {
                overworld_update_animation(em2_Buffer);
                nAnimationUpdates--;
            }                
        } else if (isr_level_state_int == ISR_LEVEL_A) {
                                    
            if (targetPos.j<map.pos.j) {
                map.pos.j--;
                overworld_draw_col(0);
                isr_level_state_int = ISR_LEVEL_A;
                nDeployISR = 2;
            } else if (targetPos.j>map.pos.j) {
                map.pos.j++;
                overworld_draw_col(31);
                isr_level_state_int = ISR_LEVEL_A;
                nDeployISR = 2;
            }
            if (targetPos.i<map.pos.i) {
                map.pos.i--;
                overworld_draw_row(0);
                map.pos.i++;
                isr_level_state_int = ISR_LEVEL_B;
                nDeployISR = 0;
            } else if (targetPos.i>map.pos.i) {
                map.pos.i++;
                overworld_draw_row(7);
                map.pos.i--;
                isr_level_state_int = ISR_LEVEL_B;
                nDeployISR = 0;
            }
            
            if (nDeployISR==0 && isr_level_state_int==ISR_LEVEL_A)
                isr_level_state = ISR_LEVEL_STANDBY;
                
        } else if (isr_level_state_int == ISR_LEVEL_B) {
            
            if (targetPos.i<map.pos.i) {
                map.pos.i--;
                overworld_draw_row(8);
                overworld_draw_row(16);
            } else if (targetPos.i>map.pos.i) {
                map.pos.i++;
                overworld_draw_row(15);
                overworld_draw_row(23);
            }
            isr_level_state_int = ISR_LEVEL_A;
            nDeployISR = 2;
        }
    }

    mapper_load_segment(oldSegmentPageB, PAGE_B);
}

int main(void) {

    // Normal initialization routine
    msxhal_init(); // Bare minimum initialization of the msx support 

    // This game has normally disabled interrupts.
    DI();  

    // Activates mode 2 and clears the screen (in black)
    TMS99X8_activateMode2(MODE2_ALL_ROWS); 
    
    mapper_load_module(overworld, PAGE_B);
    targetPos.i=0; targetPos.j=0;
    map.pos = targetPos;
    overworld_init();
    
    nDeployISR = 0;
    isr_level_state = ISR_LEVEL_STANDBY;
    isr_level_state_int = ISR_LEVEL_A;
    
    enableAnimations = true;
    msxhal_install_isr(overworld_isr);
    
    while (true) {
        
        uint8_t i=0;
        isr_level_state = ISR_LEVEL_STANDBY;
        while (i<24) {
            if (isr_level_state == ISR_LEVEL_STANDBY) {
                DI();
                TMS99X8_setRegister(7,BWhite);
                mapper_load_module(overworld, PAGE_B);
                overworld_draw_row(i);
                overworld_free();             
                i++;
                nDeployISR = 2;
                TMS99X8_setRegister(7,BBlack);
                EI();
            }
            wait_frame();
        }
        targetPos.i=0; targetPos.j=0;
        while (true) {
            wait_frame();
            if (isr_level_state == ISR_LEVEL_STANDBY) {

                uint8_t key = msxhal_joystick_read(0);

                if (key) {
                    DI();
                    if (key & J_LEFT) {
                        targetPos.j = (targetPos.j?targetPos.j-1:0);
                        isr_level_state = ISR_LEVEL_SUBMITTED;
                    }
                    if (key & J_RIGHT) {
                        targetPos.j = (targetPos.j<255-32?targetPos.j+1:255-32);
                        isr_level_state = ISR_LEVEL_SUBMITTED;
                    } 
                    if (key & J_UP) {
                        targetPos.i = (targetPos.i?targetPos.i-1:0);
                        isr_level_state = ISR_LEVEL_SUBMITTED;
                    }
                    if (key & J_DOWN) {
                        targetPos.i = (targetPos.i<127-24?targetPos.i+1:127-24);
                        isr_level_state = ISR_LEVEL_SUBMITTED;
                    }
                    if (key==J_SPACE) {
                    }
                    EI();
                }
            }
        }
    }
    
    return 0;
}
