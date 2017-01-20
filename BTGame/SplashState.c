#include "WorldView.h"
#include "Managers.h"
#include "LightGrid.h"
#include "Console.h"
#include "GameState.h"
#include "GameHelpers.h"
#include "ImageLibrary.h"
#include "MapEditor.h"
#include "DB.h"
#include "Lua.h"

#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "GameClock.h"
#include "TextArea.h"

#include "segashared\CheckedMemory.h"

#include "ImageLibrary.h"
#include "Sprites.h"
#include "AssetHelpers.h"
#include "Conways.h"

#define VP_SPEED 3
#define VP_FAST_SPEED 8

typedef enum  {
   Show,
   Conways
}SplashStates;

typedef struct {
   WorldView *view;
   bool pop;

   TextArea *select, *cont, *new, *ack, *quit, *copyright, *warning;
   ManagedImage *splash;
   Sprite *fire;

   Milliseconds StartTime, WarnTime;
   SplashStates state;

}SplashState;

static void _splashStateCreate(SplashState *state) {

   state->select = textAreaCreate(17, 17, 6, 1);
   state->cont = textAreaCreate(13, 19, 13, 1);
   state->new = textAreaCreate(15, 20, 10, 1);
   state->ack = textAreaCreate(12, 21, 66, 1);
   state->quit = textAreaCreate(13, 22, 13, 1);
   state->copyright = textAreaCreate(11, 24, 18, 1);
   state->warning = textAreaCreate(0, 0, EGA_TEXT_RES_WIDTH, EGA_TEXT_RES_HEIGHT);

   textAreaSetSpeed(state->warning, 75);


   state->state = Show;
}
static void _splashStateDestroy(SplashState *self) {
   textAreaDestroy(self->select);
   textAreaDestroy(self->cont);
   textAreaDestroy(self->new);
   textAreaDestroy(self->ack);
   textAreaDestroy(self->quit);
   textAreaDestroy(self->copyright);
   textAreaDestroy(self->warning);

   checkedFree(self);
}

static void _splashUpdate(SplashState*, GameStateUpdate*);
static void _splashHandleInput(SplashState*, GameStateHandleInput*);
static void _splashRender(SplashState*, GameStateRender*);
static void _splashEnter(SplashState*, StateEnter*);
static void _splashExit(SplashState*, StateExit*);

static void _splash(SplashState *state, Type *t, Message m) {
   if (t == GetRTTI(GameStateUpdate)) { _splashUpdate(state, m); }
   else if (t == GetRTTI(GameStateHandleInput)) { _splashHandleInput(state, m); }
   else if (t == GetRTTI(GameStateRender)) { _splashRender(state, m); }
   else if (t == GetRTTI(StateEnter)) { _splashEnter(state, m); }
   else if (t == GetRTTI(StateExit)) { _splashExit(state, m); }
}

void _splashEnter(SplashState *state, StateEnter *m) {
   WorldView *view = state->view;

   state->splash = imageLibraryGetImage(state->view->imageLibrary, stringIntern("splash.bg"));
   state->fire = spriteManagerGetSprite(view->spriteManager, stringIntern("splash.fire"));

   textAreaSetText(state->select, "Select");
   textAreaSetText(state->cont, "[i]Journey forth[/i]");
   textAreaSetText(state->new, "Begin anew");
   textAreaSetText(state->ack, "Acknowledgements");
   textAreaSetText(state->quit, "Retire to DOS");
   textAreaSetText(state->copyright, "Copyright 1988 BDT");
   
   verbManagerSetEnabled(view->verbManager, false);
   assetsSetPalette(view->db, stringIntern("splash"));

   state->StartTime = t_u2m(gameClockGetTime());

}
void _splashExit(SplashState *state, StateExit *m) {
   WorldView *view = state->view;

   spriteDestroy(state->fire);
   managedImageDestroy(state->splash);
}

void _splashUpdate(SplashState *state, GameStateUpdate *m) {
   WorldView *view = state->view;
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   cursorManagerUpdate(view->cursorManager, mousePos.x, mousePos.y);

   if (state->state == Conways) {
      textAreaUpdate(state->warning);
      if (textAreaIsDone(state->warning)) {
         if (state->WarnTime == 0) {
            state->WarnTime = t_u2m(gameClockGetTime());
         }
         else if (t_u2m(gameClockGetTime()) - state->WarnTime > 5000) {
            state->pop = true;

         }
      }
   }
   

   //if (state->state == Show) {
   //   if (t_u2m(gameClockGetTime()) - state->StartTime > 2000) {
   //      state->state = Conways;
   //   }
   //}   

   if (state->pop) {
      assetsSetPalette(state->view->db, stringIntern("default"));
      fsmPush(state->view->gameState, gameStateCreateWorld(state->view));

      luaStartup(state->view->L);
   }

}

static void _handleKeyboard(SplashState *state) {
   WorldView *view = state->view;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };


   while (keyboardPopEvent(k, &e)) {
      if (e.action == SegaKey_Released) {

         if (state->state == Show) {
            state->state = Conways;
            textAreaPushText(state->warning, 
               "[w=4][s][c=3,0] \n \n \n \n \n \n \n"
               "   Th[c=3,2]i[/c]s world [c=3,2]i[/c]s wr[c=3,2]i[/c]tten,[w=1] not der[c=3,2]i[/c]ved.[w=2]\n \n"
               "        An actor[w=1] [c=3,2]i[/c]n a dream uncar[c=3,2]i[/c]ng,[w=2]\n \n \n"
               " How w[c=3,2]i[/c]ll you l[c=3,2]i[/c]ve?[w=2]\n \n"
               "                    How w[c=3,2]i[/c]ll you d[c=3,2]i[/c]e?[w=4]\n \n \n"
               "                                -BDT[w=1]\n "               
               "[/c][/s]");
         }
         else {
            state->pop = true;
         }
         
      }
   }

}

static void _handleMouse(SplashState *state) {

}

void _splashHandleInput(SplashState *state, GameStateHandleInput *m) {
   _handleKeyboard(state);
   _handleMouse(state);
}

void _splashRender(SplashState *state, GameStateRender *m) {

   Frame *frame = m->frame;

   if (state->state == Show) {
      frameClear(frame, FrameRegionFULL, 0);
      //frameRenderImage(frame, FrameRegionFULL, 0, 0, managedImageGetImage(state->splash));

      {
         Image *splash = managedImageGetImage(state->splash);
         ManagedImage *cursor = imageLibraryGetImage(state->view->imageLibrary, stringIntern("cursor"));
         Image *crsr = managedImageGetImage(cursor);
         Texture *crsrTex = imageCreateTexture(crsr);
         Texture *splashTex = imageCreateTexture(splash);
         Texture *empty = textureCreate(EGA_RES_WIDTH, EGA_RES_HEIGHT);
         Font *testfont = fontFactoryGetFont(state->view->fontFactory, 5, 10);
         static int animateTest = 0;
         int i = ((animateTest++) / 10) % 5;
         int colorindex;

         //rendertexture
         textureRenderTexture(empty, NULL, 0, 0, splashTex);
         textureRenderTexture(empty, NULL, 100, 100, splashTex);
         textureRenderTexture(empty, NULL, 10, 10, crsrTex);

         //renderpartialtexture
         textureRenderTexturePartial(empty, NULL, 110, 110, crsrTex, 22*i, 0, 22, 33);

         //renderpoint
         for (colorindex = 0; colorindex < EGA_PALETTE_COLORS; ++colorindex) {
            textureRenderPoint(empty, NULL, 10 * colorindex, 10, colorindex);
         }

         //renderline
         for (colorindex = 0; colorindex < EGA_PALETTE_COLORS; ++colorindex) {
            textureRenderLine(empty, NULL, 100 + 10 * colorindex, 50, 100 - 10 * colorindex, 150, colorindex);
         }

         //textureClearAlpha(empty);

         //rects
         for (colorindex = 0; colorindex < EGA_PALETTE_COLORS; ++colorindex) {
            int w = EGA_RES_WIDTH / EGA_PALETTE_COLORS;
            textureRenderRect(empty, NULL, w*colorindex, EGA_RES_HEIGHT - w, w + w*colorindex, EGA_RES_HEIGHT, colorindex);
         }

         //rectLines
         for (colorindex = 0; colorindex < EGA_PALETTE_COLORS; ++colorindex) {
            int w = EGA_RES_WIDTH / EGA_PALETTE_COLORS;
            textureRenderLineRect(empty, NULL, w*colorindex, EGA_RES_HEIGHT - w*2, w + w*colorindex, EGA_RES_HEIGHT - w, colorindex);
         }

         textureRenderText(empty, "Hello world!", 10, 8, testfont);


         frameRenderTexture(frame, FrameRegionFULL, 0, 0, splashTex);
         frameRenderTexture(frame, FrameRegionFULL, 0, 0, empty);
         
         textureDestroy(empty);
         textureDestroy(crsrTex);
         textureDestroy(splashTex);
         managedImageDestroy(cursor);      
      }

      frameRenderSprite(frame, FrameRegionFULL, 79, 80, state->fire);

      textAreaRender(state->select, state->view, frame);
      textAreaRender(state->cont, state->view, frame);
      textAreaRender(state->new, state->view, frame);
      textAreaRender(state->ack, state->view, frame);
      textAreaRender(state->quit, state->view, frame);
      textAreaRender(state->copyright, state->view, frame);
   }
   else {
      //textAreaRender(state->warning, state->view, frame);
      if (!textAreaIsDone(state->warning)) {
         frameRenderSprite(frame, FrameRegionFULL, 79, 80, state->fire);
      }

      if (textAreaIsDone(state->warning)) {
         int x = appRand(appGet(), 0, EGA_RES_WIDTH);
         frameRenderRect(frame, FrameRegionFULL, x, 0, x+3, EGA_RES_HEIGHT - 1, 3);
      }
      conwaysRender(frame, FrameRegionFULL);
      textAreaRender(state->warning, state->view, frame);

      
         
      
   }


   

}

StateClosure gameStateCreateSplash(WorldView *view) {
   StateClosure out;
   SplashState *state = checkedCalloc(1, sizeof(SplashState));
   state->view = view;

   _splashStateCreate(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_splash, &_splashStateDestroy);
   return out;
}