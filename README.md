# sdcc_msx_interlacedScrollMSX1
Proof of concept of an interlaced scroll on MSX1

The main functionality of the routines lays now in mainly three files:

## makeMap

util/makeMap.cc -> reads the map image and generates severa .c files that describe the map.
Example of use:

go to the res/ folder. Use Tiled to read testMap.tmx and export an image of the map (name it testMap.png).
then run:

```
make tmp/util/makeMap && tmp/util/makeMap overworld res/animated.png res/testMap.png
```

## src/map

contains the code that updates a tile of the map, a row, or a column.

Key functions are hand optimized in assembly, but the C version equivalent is also provideed.

## src/main.c

check the main ISR function for the steps required to create the scroll.

