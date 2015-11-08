#pragma once

#include "segautils/Defs.h"
#include "segautils/Rect.h"
#include "Entities/Entities.h"

typedef struct GridManager_t GridManager;
typedef struct Frame_t Frame;
typedef struct WorldView_t WorldView;

#define GRID_POS_X 13
#define GRID_POS_Y 11

#define GRID_CELL_SIZE 14
#define GRID_WIDTH 21
#define GRID_HEIGHT 11

#define GRID_PX_WIDTH (GRID_WIDTH * GRID_CELL_SIZE)
#define GRID_PX_HEIGHT (GRID_HEIGHT * GRID_CELL_SIZE)

typedef struct OcclusionCell_t OcclusionCell;

//returns pointer to the entity array that contains all gridded entities currently in view
vec(EntityPtr) *gridManagerQueryEntities(GridManager *self);

GridManager *createGridManager(WorldView *view);
void gridManagerRender(GridManager *self, Frame *frame);
void gridManagerRenderLighting(GridManager *self, Frame *frame);
void gridManagerSetAmbientLight(GridManager *self, byte level);

short gridManagerWidth(GridManager *self);
short gridManagerHeight(GridManager *self);

//changes the schema of a given tile (world-tile coords)
void gridManagerSetTileSchema(GridManager *self, int x, int y, byte schema);

//take an area of relative to the lightgrid and a preallocated cell array (minimum size area.width * area.height)
//fills in grid occlusion levels, returns number of occluders found
int gridManagerQueryOcclusion(GridManager *self, Recti *area, OcclusionCell *grid);
