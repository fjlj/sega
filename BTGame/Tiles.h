#pragma once

#include "segautils/Defs.h"

//collision flags
#define COL_SOLID_TOP (1 << 0)
#define COL_SOLID_LEFT (1 << 1)
#define COL_SOLID_BOTTOM (1 << 2)
#define COL_SOLID_RIGHT (1 << 3)
#define COL_SOLID (COL_SOLID_TOP|COL_SOLID_LEFT|COL_SOLID_BOTTOM|COL_SOLID_RIGHT)

#pragma pack(push, 1)

typedef struct {
   byte schema;
   byte collision;//use "solid" flags
}Tile;

typedef struct {
   short img[3];
   byte imgCount;
   byte occlusion;
}TileSchema;

#pragma pack(pop)
