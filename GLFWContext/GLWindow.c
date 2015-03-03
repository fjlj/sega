#include "GLFW/glfw3.h"
#include "GLWindow.h"
#include "segashared\CheckedMemory.h"

#include <malloc.h>
#include <stddef.h>
#include <Windows.h>


struct GLWindow_T {
   GLFWwindow* window;
   Int2 winSize;
};

GLWindow *glWindowCreate(Int2 winSize, StringView windowName, GLFWmonitor *monitor){
   GLFWwindow *window;
   Int2 actualSize;
   GLWindow *r;
   
   if(!monitor) {
      glfwWindowHint(GLFW_RESIZABLE, 0);
   }

   //glfwWindowHint(GLFW_DECORATED, 0);   

   window = glfwCreateWindow(winSize.x, winSize.y, windowName, monitor, NULL);

   if(!window) {
      glfwTerminate();
      return NULL;
   }

   FreeConsole();
   
   glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
   glfwGetFramebufferSize(window, &actualSize.x, &actualSize.y);
   glfwMakeContextCurrent(window);

   r = checkedCalloc(1, sizeof(GLWindow));
   r->window = window;
   r->winSize = actualSize;   

   return r;
}
void glWindowDestroy(GLWindow *self){
   glfwDestroyWindow(self->window);
   glfwTerminate();
   checkedFree(self);
}

void glWindowPollEvents(GLWindow *self){
   glfwPollEvents();
}
void glWindowSwapBuffers(GLWindow *self){
   glfwSwapBuffers(self->window);
}
int glWindowShouldClose(GLWindow *self){
   return glfwWindowShouldClose(self->window);
}
Int2 glWindowGetSize(GLWindow *self){
   return self->winSize;
}
Float2 glWindowGetMousePos(GLWindow *self){
   double x, y;
   glfwGetCursorPos(self->window, &x, &y);
   return float2Create((float)x, (float)y);
}