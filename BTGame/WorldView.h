#pragma once

#include "Viewport.h"

typedef struct BTManagers_t BTManagers;
typedef struct EntitySystem_t EntitySystem;
typedef struct Entity_t Entity;
typedef struct ImageLibrary_t ImageLibrary;
typedef struct FSM_t FSM;
typedef struct GameClock_t GameClock;
typedef struct GridSolver_t GridSolver;
typedef struct Console_t Console;
typedef struct lua_State lua_State;

typedef struct WorldView_t {
   BTManagers *managers;
   EntitySystem *entitySystem;
   Entity *backgroundEntity;
   ImageLibrary *imageLibrary;
   FSM *gameState;
   GameClock *gameClock;
   GridSolver *gridSolver;
   Viewport *viewport;
   Console *console;
   lua_State *L;
}WorldView;