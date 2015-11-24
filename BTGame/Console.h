#pragma once

#include "segautils/Defs.h"
#include "SEGA/Input.h"
#include "WorldView.h"

typedef struct Frame_t Frame;
typedef struct Console_t Console;

Console *consoleCreate(WorldView *view);
void consoleDestroy(Console *self);

void consoleCreateLines(Console *self);
void consoleSetEnabled(Console *self, bool enabled);
bool consoleGetEnabled(Console *self);
void consoleInputChar(Console *self, char c);
void consoleInputKey(Console *self, SegaKeys key);
void consolePushLine(Console *self, const char *line);