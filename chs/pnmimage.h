#ifndef pnmimageH
#define pnmimageH

#include <stdio.h>

#include "image.h"
#include "macros.h"

class cPnmImage : public cImage
{
 public:
  cPnmImage(const char *name, int width, int height, tColor bg = tColor::WHITE);
  virtual ~cPnmImage();

  virtual void close();

  /** Pixel coordinates are (x,y) in (0..width-1, 0..height-1). */
  virtual void set_row(int x, tColor color);
  virtual void set_pixel(int x, int y, tColor color);

 private:
  DISALLOW_COPY_AND_ASSIGN(cPnmImage);

  FILE *fp;
  const long meta_offset;
  const int width;
  const int height;
};

#endif

