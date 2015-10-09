#include "BT.h"
#include "SEGA\App.h"
#include "segashared\CheckedMemory.h"
#include "segashared\Strings.h"

#include "Entities\Entities.h"
#include "WorldView.h"
#include "GameClock.h"
#include "ImageLibrary.h"
#include "Managers.h"
#include "GameState.h"
#include "GridManager.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define FULLSCREEN 1
#define FRAME_RATE 24

typedef struct {
   VirtualApp vApp;
   AppData data;

   BTManagers managers;

   EntitySystem *entitySystem;
   ImageLibrary *imageLibrary;
   FSM *gameState;
   GameClock *gameClock;
   WorldView view;

} BTGame;

#pragma region App_Things

static AppData *_getData(BTGame *);
static void _destroy(BTGame *);
static void _onStart(BTGame *);
static void _onStep(BTGame *);

static VirtualAppVTable *getVtable()
{
   static VirtualAppVTable *vtable;
   if (!vtable) {
      vtable = malloc(sizeof(VirtualAppVTable));
      vtable->getData = (AppData *(*)(VirtualApp *))&_getData;
      vtable->destroy = (void(*)(VirtualApp *))&_destroy;
      vtable->onStart = (void(*)(VirtualApp *))&_onStart;
      vtable->onStep = (void(*)(VirtualApp *))&_onStep;
   }

   return vtable;
}
AppData createData() {
   AppData data = { 0 };

   data.defaultWindowSize = (Int2){ WINDOW_WIDTH, WINDOW_HEIGHT };
   data.frameRate = FRAME_RATE;

   if (FULLSCREEN){
      data.dcFlags |= DC_FLAG_FULLSCREEN;
   }

   data.windowTitle = stringIntern("sEGA: An elegant weapon for a more civilized age.");

   return data;
}
AppData *_getData(BTGame *self) {
   return &self->data;
}

#pragma endregion

#define RegisterManager(member, funcCall) \
   member = funcCall; \
   entitySystemRegisterManager(self->entitySystem, (Manager*)member);

void _initEntitySystem(BTGame *self){
   self->entitySystem = entitySystemCreate();
   self->view.entitySystem = self->entitySystem;//this is important

   RegisterManager(self->managers.renderManager, createRenderManager(&self->view, &self->data.fps));
   RegisterManager(self->managers.cursorManager, createCursorManager(&self->view));
   RegisterManager(self->managers.gridManager, createGridManager(&self->view));
}

void _destroyEntitySystem(BTGame *self){
   entitySystemDestroy(self->entitySystem);
}

VirtualApp *btCreate() {
   BTGame *r = checkedCalloc(1, sizeof(BTGame));
   r->vApp.vTable = getVtable();
   r->data = createData();

   //Other constructor shit goes here   
   r->imageLibrary = imageLibraryCreate();
   r->gameState = fsmCreate();
   r->gameClock = gameClockCreate();

   //init the view
   r->view.imageLibrary = r->imageLibrary;
   r->view.gameState = r->gameState;
   r->view.managers = &r->managers;
   r->view.gameClock = r->gameClock;
   r->view.viewport = (Viewport){ .screen = { GRID_POS_X, GRID_POS_Y }, 
                                  .world = rectiCreate(0, 0, GRID_PX_WIDTH, GRID_PX_HEIGHT) };

   _initEntitySystem(r);

   return (VirtualApp*)r;
}

void _destroy(BTGame *self){
   fsmDestroy(self->gameState);
   _destroyEntitySystem(self);

   imageLibraryDestroy(self->imageLibrary);
   gameClockDestroy(self->gameClock);
   checkedFree(self);
}

void _onStart(BTGame *self){
   //Entity *e = entityCreate(self->entitySystem);
   //COMPONENT_ADD(e, PositionComponent, 0, 0);
   //COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/dotagrid.ega"));
   //entityUpdate(e);
   
   appLoadPalette(appGet(), "assets/img/default.pal");
   //appLoadPalette(appGet(), "assets/img/default.pal");
   cursorManagerCreateCursor(self->managers.cursorManager);

   //push the opening state
   fsmPush(self->gameState, gameStateCreateWorld(&self->view));

}


void _onStep(BTGame *self){
   fsmSend(self->gameState, GameStateHandleInput);
   fsmSend(self->gameState, GameStateUpdate);
   fsmSendData(self->gameState, GameStateRender, self->vApp.currentFrame);
}



