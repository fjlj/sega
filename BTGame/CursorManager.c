#include "Managers.h"

#include "Managers.h"
#include "ImageLibrary.h"
#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "segautils\Rect.h"
#include "segautils\Defs.h"

#include <stdio.h>

typedef struct{
   byte flag;
}TCursorComponent;

#define TComponentT TCursorComponent
#include "Entities\ComponentDeclTransient.h"

struct CursorManager_t{
   Manager m;
   EntitySystem *system;
   Entity *e;

   Int2 dragStart;

   byte boxColor;

   vec(Int2) *dragBox;
};

ImplManagerVTable(CursorManager)

CursorManager *createCursorManager(EntitySystem *system){
   CursorManager *out = checkedCalloc(1, sizeof(CursorManager));
   out->system = system;
   out->m.vTable = CreateManagerVTable(CursorManager);

   out->dragBox = vecCreate(Int2)(NULL);
   vecResize(Int2)(out->dragBox, 4, &(Int2){0, 0});

   out->boxColor = 15;

   return out;
}

void _destroy(CursorManager *self){
   vecDestroy(Int2)(self->dragBox);
   checkedFree(self);
}
void _onDestroy(CursorManager *self, Entity *e){}
void _onUpdate(CursorManager *self, Entity *e){}

static void _updateDragBox(CursorManager *self, int x, int y){

   *vecAt(Int2)(self->dragBox, 0) = (Int2){ self->dragStart.x, self->dragStart.y };
   *vecAt(Int2)(self->dragBox, 1) = (Int2){ x, self->dragStart.y };
   *vecAt(Int2)(self->dragBox, 2) = (Int2){ x, y };
   *vecAt(Int2)(self->dragBox, 3) = (Int2){ self->dragStart.x, y };

}

void cursorManagerCreateCursor(CursorManager *self){
   self->e = entityCreate(self->system);

   COMPONENT_ADD(self->e, PositionComponent, 0, 0);
   COMPONENT_ADD(self->e, ImageComponent, stringIntern("assets/img/cursor.ega"));
   COMPONENT_ADD(self->e, TCursorComponent, 0);
   COMPONENT_ADD(self->e, LayerComponent, LayerUI);   
   entityUpdate(self->e);
}

void cursorManagerStartDrag(CursorManager *self, int x, int y){
   self->dragStart = (Int2){ x, y };
   _updateDragBox(self, x, y);
   COMPONENT_ADD(self->e, PolygonComponent, .pList = self->dragBox, .color = self->boxColor, .open = false);
   entityUpdate(self->e);
}

Recti cursorManagerEndDrag(CursorManager *self, int x, int y){
   entityRemove(PolygonComponent)(self->e);
   entityUpdate(self->e);
   return (Recti){ 
         MIN(self->dragStart.x, x),
         MIN(self->dragStart.y, y),
         MAX(self->dragStart.x, x),
         MAX(self->dragStart.y, y)
   };
}

void cursorManagerUpdate(CursorManager *self, int x, int y){
   PositionComponent *pc = entityGet(PositionComponent)(self->e);
   if (pc){
      pc->x = x;
      pc->y = y;
   }

   _updateDragBox(self, x, y);
}
