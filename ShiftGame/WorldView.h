#pragma once

typedef struct ShiftManagers_t ShiftManagers;
typedef struct EntitySystem_t EntitySystem;
typedef struct ImageLibrary_t ImageLibrary;
typedef struct FSM_t FSM;
typedef struct GameClock_t GameClock;

typedef struct WorldView_t {
   ShiftManagers *managers;
   EntitySystem *entitySystem;
   ImageLibrary *imageLibrary;
   FSM *gameState;
   GameClock *gameClock;
}WorldView;