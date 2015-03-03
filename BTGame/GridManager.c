#include "Managers.h"

#include "Managers.h"
#include "ImageManager.h"

#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "segautils\IntrusiveHeap.h"
#include "SEGA\App.h"
#include "GridManager.h"
#include "segautils\StandardVectors.h"

#include <stdio.h>

#define ClosureTPart \
    CLOSURE_RET(size_t) /*return edge*/\
    CLOSURE_NAME(GridProcessNeighbor) \
    CLOSURE_ARGS(GridNodePublic */*current*/, GridNodePublic*/*neighbor*/)
#include "segautils\Closure_Impl.h"

#define ClosureTPart \
    CLOSURE_RET(int) /*return true if solved*/ \
    CLOSURE_NAME(GridProcessCurrent) \
    CLOSURE_ARGS(GridNodePublic*/*current*/)
#include "segautils\Closure_Impl.h"

#define VectorTPart GridSolutionNode
#include "segautils\Vector_Impl.h"

typedef struct GridNode_t GridNode;

struct GridNode_t{
   GridNodePublic data;
   QueueNode node;
   byte visited;
   GridNode *parent;
   GridNode *neighbors[4];
   size_t score;
};

size_t gridNodeGetScore(GridNodePublic *self){
   return ((GridNode*)self)->score;
}

static GridNode *_nodeCompareFunc(GridNode *n1, GridNode *n2){
   return n1->score < n2->score ? n1 : n2;
}

#pragma region Dijkstra's'
typedef struct {
   Dijkstras inner;
   GridProcessCurrent cFunc;
   GridProcessNeighbor nFunc;
   
}GridSolver;

static size_t _getNeighbors(GridSolver *self, GridNode *node, GridNode ***outList);
static int _processNeighbor(GridSolver *self, GridNode *current, GridNode *node);
static int _processCurrent(GridSolver *self, GridNode *node);
static void _destroy(GridSolver *self);

static DijkstrasVTable *_getVTable(){
   DijkstrasVTable *out = NULL;
   if (!out){
      out = calloc(1, sizeof(DijkstrasVTable));
      out->getNeighbors = (size_t(*)(Dijkstras*, QueueElem, QueueElem**))&_getNeighbors;
      out->processNeighbor = (int(*)(Dijkstras*, QueueElem, QueueElem))&_processNeighbor;
      out->processCurrent = (int(*)(Dijkstras*, QueueElem))&_processCurrent;
      out->destroy = (void(*)(Dijkstras*))&_destroy;
   }
   return out;
}

size_t _getNeighbors(GridSolver *self, GridNode *node, GridNode ***outList){
   *outList = node->neighbors;
   return 4;
}
int _processNeighbor(GridSolver *self, GridNode *current, GridNode *node){
   if (node && !node->visited){
      size_t newScore = closureCall(&self->nFunc, &current->data, &node->data);
      if (newScore < node->score){
         node->parent = current;
         node->score = newScore;
         return true;
      }
   }
   return false;
}
int _processCurrent(GridSolver *self, GridNode *node){
   node->visited = true;
   return closureCall(&self->cFunc, &node->data);
}
void _destroy(GridSolver *self){
   priorityQueueDestroy(self->inner.queue);
   checkedFree(self);
}

#pragma endregion


typedef struct{
   vec(size_t) *occupyingNodes;
}TGridComponent;

#define TComponentT TGridComponent
#include "Entities\ComponentDeclTransient.h"

struct GridManager_t{
   Manager m;
   EntitySystem *system;

   GridNode table[CELL_COUNT];
   vec(GridSolutionNode) *solutionMap;
};

static void _addNeighbors(GridNode *nodes, size_t current){
   int x, y, i;
   size_t nID[4];
   GridNode *node = nodes + current;
   gridXYFromIndex(node->data.ID, &x, &y);

   nID[0] = gridIndexFromXY(x, y - 1);
   nID[1] = gridIndexFromXY(x - 1, y);
   nID[2] = gridIndexFromXY(x + 1, y);
   nID[3] = gridIndexFromXY(x, y + 1);

   for (i = 0; i < 4; ++i){
      if (nID[i] < INF){
         node->neighbors[i] = nodes + nID[i];
      }
   }
}

static void _buildTable(GridNode *nodes){
   size_t i;
   for (i = 0; i < CELL_COUNT; ++i){
      nodes[i].data.ID = i;
      nodes[i].score = INF;
      nodes[i].data.entities = vecCreate(EntityPtr)(NULL);
      _addNeighbors(nodes, i);

   }
}

static void _clearTable(GridNode *nodes){
   int i;
   for (i = 0; i < CELL_COUNT; ++i){
      nodes[i].score = INF;
      nodes[i].visited = false;
      nodes[i].parent = NULL;
      queueNodeClear(&nodes[i].node);
   }
}

static void _clearTableEntities(GridNode *nodes){
   int i;
   for (i = 0; i < CELL_COUNT; ++i){
      vecClear(EntityPtr)(nodes[i].data.entities);
   }
}

#pragma region vtable things
static void GridManagerDestroy(GridManager*);
static void GridManagerOnDestroy(GridManager*, Entity*);
static void GridManagerOnUpdate(GridManager*, Entity*);

static ManagerVTable *_createVTable(){
   static ManagerVTable *out = NULL;

   if (!out){
      out = calloc(1, sizeof(ManagerVTable));
      out->destroy = (void(*)(Manager*))&GridManagerDestroy;
      out->onDestroy = (void(*)(Manager*, Entity*))&GridManagerOnDestroy;
      out->onUpdate = (void(*)(Manager*, Entity*))&GridManagerOnUpdate;
   }

   return out;
}

#pragma endregion

GridManager *createGridManager(EntitySystem *system){
   GridManager *out = checkedCalloc(1, sizeof(GridManager));
   out->system = system;
   out->m.vTable = _createVTable();
   out->solutionMap = vecCreate(GridSolutionNode)(NULL);

   _buildTable(out->table);

   return out;
}

void GridManagerDestroy(GridManager *self){
   size_t i;
   for (i = 0; i < CELL_COUNT; ++i){
      vecDestroy(EntityPtr)(self->table[i].data.entities);
   }
   vecDestroy(GridSolutionNode)(self->solutionMap);
   checkedFree(self);
}

void GridManagerOnDestroy(GridManager *self, Entity *e){
   TGridComponent *tgc = entityGet(TGridComponent)(e);
   if (tgc){
      vecDestroy(size_t)(tgc->occupyingNodes);
   }
}
void GridManagerOnUpdate(GridManager *self, Entity *e){
   TGridComponent *tgc = entityGet(TGridComponent)(e);
   GridComponent *gc = entityGet(GridComponent)(e);

   if (gc){
      if (!tgc){
         //new grid entry
         ADD_NEW_COMPONENT(e, TGridComponent, vecCreate(size_t)(NULL));
      }
   }
   else{
      if (tgc){
         //no longer rendered
         vecDestroy(size_t)(tgc->occupyingNodes);
         entityRemove(TGridComponent)(e);
      }
   }
}

GridSolution gridManagerSolve(GridManager *self, size_t startCell, GridProcessCurrent cFunc, GridProcessNeighbor nFunc){
   GridSolution solution = { INF, INF, NULL };

   if (startCell < CELL_COUNT){
      int i;
      GridNode *result = NULL;
      PriorityQueue *pq = priorityQueueCreate(offsetof(GridNode, node), (PQCompareFunc)&_nodeCompareFunc);
      GridSolver *dk = checkedCalloc(1, sizeof(GridSolver));
      _clearTable(self->table);
      self->table[startCell].score = 0;

      for (i = 0; i < CELL_COUNT; ++i){
         priorityQueuePush(pq, self->table + i);
      }

      dk->inner.vTable = _getVTable();
      dk->inner.queue = pq;
      dk->cFunc = cFunc;
      dk->nFunc = nFunc;

      result = dijkstrasRun((Dijkstras*)dk);

      if (result){
         solution.solutionCell = result->data.ID;
         solution.totalCost = result->score;

         //self->solutionMap = vecCreate(GridSolutionNode)(NULL);
         vecClear(GridSolutionNode)(self->solutionMap);

         while (result && result->parent){
            GridSolutionNode resoluteNode = { .node = result->data.ID };
            vecPushBack(GridSolutionNode)(self->solutionMap, &resoluteNode);
            result = result->parent;
         }

         vecReverse(GridSolutionNode)(self->solutionMap);
         solution.path = self->solutionMap;
      }
      dijkstrasDestroy((Dijkstras*)dk);
   }

   

   return solution;
}

vec(EntityPtr) *gridManagerEntitiesAt(GridManager *self, size_t index){
   if (index < CELL_COUNT){
      return self->table[index].data.entities;
   }
   else {
      return NULL;
   }
}

static void _updateEntity(GridManager *self, Entity *e){
   TGridComponent *tgc = entityGet(TGridComponent)(e);
   GridComponent *gc = entityGet(GridComponent)(e);
   if (tgc && gc){
      size_t pos = gridIndexFromXY(gc->x, gc->y);

      vecClear(size_t)(tgc->occupyingNodes);

      vecPushBack(size_t)(tgc->occupyingNodes, &pos);
      vecPushBack(EntityPtr)(gridManagerEntitiesAt(self, pos), &e);
   }
}

void gridManagerUpdate(GridManager *self){
   _clearTableEntities(self->table);

   COMPONENT_QUERY(self->system, GridComponent, gc, {
      Entity *e = componentGetParent(gc, self->system);
      _updateEntity(self, e);
   });
}
