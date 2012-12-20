#ifndef imageH
#define imageH

#include "macros.h"

struct tColor
{
  inline tColor(unsigned char r, unsigned char g, unsigned char b)
      : r(r), g(g), b(b) {}

  unsigned char r;
  unsigned char g;
  unsigned char b;

  static const tColor WHITE;
  static const tColor GRAY;
  static const tColor BLACK;
  static const tColor RED;
  static const tColor GREEN;
  static const tColor BLUE;
};

class cImage
{
 public:
  cImage() {}
  virtual ~cImage();

  virtual void close() = 0;

  virtual void set_row(int x, tColor color) = 0;
  virtual void set_pixel(int x, int y, tColor color) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(cImage);
};

#endif

