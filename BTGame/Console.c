#include "Console.h"
#include "segalib/EGA.h"
#include "Entities/Entities.h"
#include "segashared/CheckedMemory.h"
#include "CoreComponents.h"

#include "segautils/StandardVectors.h"
#include "GameClock.h"
#include "Managers.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"
#include "Lua.h"

#include "RichText.h"

#define LINE_COUNT 17
#define WIDTH 36
#define BOTTOM 19
#define LEFT 2

#define PREFIX     "> "
#define PREFIX_LEN 2
#define SHOWN_INPUT_LEN (WIDTH - PREFIX_LEN - 1)

struct Console_t {
   WorldView *view;
   Entity *e;
   bool enabled;
   vec(StringPtr) *inputHistory;   
   String *input;

   RichText *rt;
   vec(RichTextLine) *queue;

   int historyLocation;
   int cursorPos, selectionWidth;
   int skippedChars;
   Microseconds cursorClock;
   bool invertCursor;
};

static int lsega_playerMove(lua_State *L) {
   int x = (int)luaL_checkinteger(L, 1);
   int y = (int)luaL_checkinteger(L, 2);
   WorldView *view = luaGetWorldView(L);

   pcManagerMove(view->managers->pcManager, x, y);

   return 0;
}


Console *consoleCreate(WorldView *view) {
   Console *out = checkedCalloc(1, sizeof(Console));
   out->view = view;
   out->queue = vecCreate(RichTextLine)(&richTextLineDestroy);
   out->inputHistory = vecCreate(StringPtr)(&stringPtrDestroy);
   out->input = stringCreate("");
   out->rt = richTextCreateFromRaw("");

   return out;
}
void consoleDestroy(Console *self) {

   richTextDestroy(self->rt);
   stringDestroy(self->input);
   vecDestroy(RichTextLine)(self->queue);
   vecDestroy(StringPtr)(self->inputHistory);
   checkedFree(self);
}

static RichTextLine _inputLine(Console *self) {
   return (vecEnd(TextLine)(entityGet(TextComponent)(self->e)->lines) - 1)->line;
}

static void _updateInputLine(Console *self) {

   int inlen = stringLen(self->input);
   int len = MIN(SHOWN_INPUT_LEN, inlen);
   int skippedChars = self->skippedChars;
   int cursorPos = self->cursorPos - skippedChars;
   RichTextLine input = _inputLine(self);
   String *innerString = stringCreate("");

   stringSet(innerString, PREFIX);
   stringConcat(innerString, "[=]");
   
   if (self->cursorPos == inlen) {
      //cursor is at the end so draw the whole string and a highlighted space
      stringConcat(innerString, c_str(self->input) + skippedChars);

      if (self->invertCursor) {
         stringConcat(innerString, "[/=][i] [/i][=]");
      }
   }
   else {
      //cusor is mid-word so split it upand highlight the cusor pos
      stringConcatEX(innerString, c_str(self->input) + skippedChars, cursorPos);
      if (self->invertCursor) {
         stringConcat(innerString, "[/=][i]");
      }
      stringConcatEX(innerString, c_str(self->input) + skippedChars + cursorPos, 1);
      if (self->invertCursor) {
         stringConcat(innerString, "[/i][=]");
      }

      stringConcatEX(innerString, c_str(self->input) + skippedChars + cursorPos + 1, MAX(0, len - cursorPos - 1));
   }

   stringConcat(innerString, "[/=]");

   richTextReset(self->rt, innerString);

   vecClear(Span)(input);
   richTextLineCopy(richTextGetSpans(self->rt), input);  
}

static void _updateConsoleLines(Console *self) {
   size_t i;
   size_t queuelen = vecSize(RichTextLine)(self->queue);
   size_t drawnCount = MIN(LINE_COUNT, queuelen);
   size_t skipCount = LINE_COUNT - drawnCount;
   TextComponent *tc = entityGet(TextComponent)(self->e);
   
   for (i = 0; i < LINE_COUNT; ++i) {
      TextLine *line = vecAt(TextLine)(tc->lines, i + 1);
      vecClear(Span)(line->line);
      if (i >= skipCount) {
         //render the spans onto the component
         RichTextLine queueline = *(vecEnd(RichTextLine)(self->queue) - (LINE_COUNT - skipCount) + (i - skipCount));

         //push the queueline spans into the console entity
         richTextLineCopy(queueline, line->line);
      }
   }
}

static void _printStackItem(lua_State *L, Console *self, int index) {
   int t = lua_type(L, index);
   switch (t) {

   case LUA_TSTRING:  /* strings */
      consolePrintLine(self, "Returned: [c=0,5]'%s'[/c]", lua_tostring(L, index));
      break;

   case LUA_TBOOLEAN:  /* booleans */
      consolePrintLine(self, "Returned: [c=0,5]%s[/c]", lua_toboolean(L, index) ? "true" : "false");
      break;

   case LUA_TNUMBER:  /* numbers */
      consolePrintLine(self, "Returned: [c=0,5]%g[/c]", lua_tonumber(L, index));
      break;

   default:  /* other values */
      consolePrintLine(self, "Returned: [c=0,5]%s[/c]", lua_typename(L, t));
      break;
   }
}

static void _processInput(Console *self, String *input) {
   int error;
   lua_State *L = self->view->L;

   consolePrintLine(self, "> %s", c_str(input));
   error = luaL_loadbuffer(L, c_str(input), stringLen(input), "line") || lua_pcall(L, 0, LUA_MULTRET, 0);

   if (error) {
      consolePrintLine(self, "[c=0,13]%s[/c]", lua_tostring(L, -1));
      lua_pop(L, 1);
   }
   else {
      while (lua_gettop(L)) {
         _printStackItem(L, self, 1);
         lua_pop(L, 1);
      }
   }
}

static void _commitInput(Console *self) {
   String *input;
   
   if (stringLen(self->input) == 0) {
      //input is empty, do nothing
      return;
   }
   //copy out the input
   input = stringCopy(self->input);

   //process the input...
   _processInput(self, input);
   
   //clear the console
   stringClear(self->input);
   self->cursorPos = 0;
   self->skippedChars = 0;
   _updateInputLine(self);

   //update the history
   vecPushBack(StringPtr)(self->inputHistory, &input);
   self->historyLocation = vecSize(StringPtr)(self->inputHistory);
   
}

void consoleCreateLines(Console *self) {
   int y;
   Entity *e = entityCreate(self->view->entitySystem);
   TextComponent tc = { .lines = vecCreate(TextLine)(&textLineDestroy) };
   int bottomLine = BOTTOM;//last line on screen
   int topLine = bottomLine - LINE_COUNT - 1;//account for input line

   for (y = topLine; y <= bottomLine; ++y) {
      vecPushBack(TextLine)(tc.lines, &(TextLine){
         .x = LEFT, .y = y,
         .line = vecCreate(Span)(&spanDestroy)
      });
   }

   self->e = entityCreate(self->view->entitySystem);
   COMPONENT_ADD(self->e, LayerComponent, LayerConsole);
   COMPONENT_ADD(self->e, RenderedUIComponent, 0);
   COMPONENT_ADD(self->e, VisibilityComponent, .shown = self->enabled);
   entityAdd(TextComponent)(self->e, &tc);
   entityUpdate(self->e);

   consoleClear(self);
   _updateInputLine(self);
}

void consoleClear(Console *self) {
   vecClear(RichTextLine)(self->queue);
   consolePushLine(self,
      "---------------------------\n"
      "| [i][c=0,14]Welcome[/i] to the console![/c] |\n"
      "---------------------------");

   consolePushLine(self, "");
}

void consoleSetEnabled(Console *self, bool enabled) {
   self->enabled = enabled;
   entityGet(VisibilityComponent)(self->e)->shown = enabled;
}

bool consoleGetEnabled(Console *self) {
   return self->enabled;
}

static void _cursorMove(Console *self, int delta) {
   self->cursorPos = MIN(stringLen(self->input), MAX(0, self->cursorPos + delta));
   self->invertCursor = true;
   self->skippedChars = MIN(self->cursorPos, self->skippedChars);

   if (self->cursorPos >= self->skippedChars + SHOWN_INPUT_LEN) {
      self->skippedChars = self->cursorPos - SHOWN_INPUT_LEN;
   }
}

void consoleInputChar(Console *self, char c) {
   stringInsert(self->input, c, self->cursorPos);
   _cursorMove(self, 1);
   _updateInputLine(self);
}
static void _historyUp(Console *self) {
   size_t historyLen = vecSize(StringPtr)(self->inputHistory);

   if (historyLen > 0 && self->historyLocation > 0) {
      --self->historyLocation;
      stringSet(self->input, c_str(*vecAt(StringPtr)(self->inputHistory, self->historyLocation)));

      self->cursorPos = stringLen(self->input);
      if (self->cursorPos > self->skippedChars + SHOWN_INPUT_LEN) {
         self->skippedChars = self->cursorPos - SHOWN_INPUT_LEN;
      }
      self->invertCursor = true;
      _updateInputLine(self);
   }
}
static void _historyDown(Console *self) {
   int historyLen = vecSize(StringPtr)(self->inputHistory);

   if (historyLen > 0){
      if (self->historyLocation < historyLen - 1) {
         ++self->historyLocation;
         stringSet(self->input, c_str(*vecAt(StringPtr)(self->inputHistory, self->historyLocation)));         
      }
      else if (self->historyLocation == historyLen - 1){
         ++self->historyLocation;
         stringClear(self->input);
      }

      self->cursorPos = stringLen(self->input);
      if (self->cursorPos > self->skippedChars + SHOWN_INPUT_LEN) {
         self->skippedChars = self->cursorPos - SHOWN_INPUT_LEN;
      }
      self->invertCursor = true;
      _updateInputLine(self);
   }
}

static void _backspace(Console *self) {
   size_t len = stringLen(self->input);
   if (len > 0 && self->cursorPos > 0) {

      if (self->cursorPos == len) {
         stringErase(self->input, len - 1);
      }
      else {
         stringErase(self->input, self->cursorPos - 1);
      }
      
      _cursorMove(self, -1);
      _updateInputLine(self);
   }
}
static void _delete(Console *self) {
   size_t len = stringLen(self->input);
   if (len > 0 && self->cursorPos < len) {
      stringErase(self->input, self->cursorPos);
      _updateInputLine(self);
   }
}

void consoleInputKey(Console *self, SegaKeys key, SegaKeyMods mods) {
   switch (key) {
   case SegaKey_Enter:
      _commitInput(self);
      break;
   case SegaKey_Up:
      _historyUp(self);
      break;
   case SegaKey_Down:
      _historyDown(self);      
      break;
   case SegaKey_Left:
      _cursorMove(self, -1);
      _updateInputLine(self);
      break;
   case SegaKey_Right:
      _cursorMove(self, 1);
      _updateInputLine(self);
      break;
   case SegaKey_Backspace:
      _backspace(self);
      break;
   case SegaKey_Delete:
      _delete(self);
      break;
   case SegaKey_Home:
      self->cursorPos = 0;
      self->skippedChars = 0;
      self->invertCursor = true;
      _updateInputLine(self);
      break;
   case SegaKey_End:
      self->cursorPos = stringLen(self->input);
      if (self->cursorPos > self->skippedChars + SHOWN_INPUT_LEN) {
         self->skippedChars = self->cursorPos - SHOWN_INPUT_LEN;
      }
      self->invertCursor = true;
      _updateInputLine(self);
      break;
   }
}

void consolePushLine(Console *self, const char *line) {
   richTextResetFromRaw(self->rt, line);
   richTextRenderToLines(self->rt, WIDTH, self->queue); 
   _updateConsoleLines(self);
}

void consoleUpdate(Console *self) {
   if (!self->cursorClock) {
      self->cursorClock = gameClockGetTime(self->view->gameClock);
   }
   else if (gameClockGetTime(self->view->gameClock) > self->cursorClock) {
      self->cursorClock += t_m2u(500);
      self->invertCursor = !self->invertCursor;
      _updateInputLine(self);
   }
}