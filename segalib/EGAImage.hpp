#pragma once

#include <direct.h>
#include <stdio.h>

#include <algorithm>
#include <memory>

#include "EGA.h"

class EGAImage
{
   class Impl;
   std::unique_ptr<Impl> pImpl;
public:
   EGAImage(const char *file);
   EGAImage(Image *image, byte *palette);
   ~EGAImage();

   short width();
   short height();

   void renderWithPalette(byte *p);

   byte *palette();
   Image *toImage();

   void exportPNG(const char*path);
};
