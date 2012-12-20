#include "pnmimage.h"

#include <strings.h>

#include "util.h"

namespace {

size_t rowbytes(int width) {
  return (size_t) width * 3;
}

FILE *init_image(const char *name, int width, int height, tColor bg)
{
  FILE *fp = fopen_next(name, "pnm");
  if (fp) {
    const char *magic = "P6";
    const int depth = 255;
    char str[64];
    size_t len, written;

    sprintf(str, "%s\n", magic);
    len = strlen(str);
    written = fwrite(str, sizeof(char), len, fp);
    assert(written == len);

    sprintf(str, "%d %d\n", width, height);
    len = strlen(str);
    written = fwrite(str, sizeof(char), len, fp);
    assert(written == len);

    sprintf(str, "%d\n", depth);
    len = strlen(str);
    written = fwrite(str, sizeof(char), len, fp);
    assert(written == len);
  }
  return fp;
}

}

cPnmImage::cPnmImage(const char *name, int width, int height, tColor bg)
  : fp(init_image(name, width, height, bg)),
    meta_offset((fp != NULL) ? ftell(fp) : -1),
    width(width),
    height(height)
{
  if (!fp) {
    fprintf(stderr, "could not open image %s\n", name);
  }
  for (int i = 0; i < height; ++i) {
    set_row(i, bg);
  }
}

cPnmImage::~cPnmImage()
{
  close();
}

void cPnmImage::close()
{
  if (fp) fclose(fp);
  fp = NULL;
}

void cPnmImage::set_row(int x, tColor color)
{
  if (!fp) {
    fprintf(stderr, "image was not opened\n");
    return;
  }
  unsigned char *buf = new unsigned char[3 * width];
  for (int i = 0; i < 3 * width; ) {
    buf[i++] = color.r;
    buf[i++] = color.g;
    buf[i++] = color.b;
  }
  size_t written = fwrite(buf, sizeof(char), rowbytes(width), fp);
  delete[] buf;
  assert(written == rowbytes(width));
}

void cPnmImage::set_pixel(int x, int y, tColor color)
{
  if (!fp) {
    fprintf(stderr, "image was not opened\n");
    return;
  }
  char buf[3];
  buf[0] = color.r;
  buf[1] = color.g;
  buf[2] = color.b;

  const long pixel_offset = rowbytes(width) * y + rowbytes(x);
  fseek(fp, meta_offset + pixel_offset, SEEK_SET);

  size_t written = fwrite(buf, sizeof(char), sizeof(buf), fp);
  assert(written == sizeof(buf));
}

