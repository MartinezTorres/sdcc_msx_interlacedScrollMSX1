NAME := weiss

include sdcc_msx/Makefile.in

CCFLAGS_MSX   += -DKONAMI5
CCFLAGS_MSX   += --all-callee-saves --opt-code-speed
#CCFLAGS_MSX   += --reserve-regs-iy --callee-saves-bc

ADDR_DATA = 0xC000

CCZ80 = sdcc_msx/bin/sdcc-3.9.0/bin/sdcc

MAX_ALLOCS = 20000
