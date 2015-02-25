#pragma once

#include "Entities\Entities.h"
#include "segashared\Strings.h"



typedef struct {
   int x, y;
}PositionComponent;

#define ComponentT PositionComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   int x, y;
}VelocityComponent;

#define ComponentT VelocityComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   StringView filename;
}ImageComponent;

#define ComponentT ImageComponent
#include "Entities\ComponentDecl.h"