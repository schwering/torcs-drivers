#include "worldmodel.h"

#include <sys/select.h>

#include <raceinit.h>
#include <tgfclient.h>

#include "macros.h"
#include "util.h"

int cWorldModel::priority() const {
  return 10000;
}

void cWorldModel::handle(cDriver& state)
{
  for (int i = 0; i < state.sit->_ncars; ++i) {
    const double now = state.sit->currentTime;
    /*
    tCarElt* car = state.sit->cars[i];
    if (times.find(car->index) == times.end() ||
        now - times[car->index] > 0.5) {
      fireEvents(now, state.sit->cars[i]);
      times[car->index] = now;
    }
    */
    fireEvents(now, state.sit->cars[i]);
  }
}

void cWorldModel::fireEvents(double now, tCarElt* car)
{
  tCarInfo ci;
  ci.name = car->_name;
  ci.time = now;
  ci.veloc = car->_speed_x;
  ci.accel = car->_accel_x;
  const tTrkLocPos trkPos = car->_trkPos;
  ci.yaw = car->_yaw - RtTrackSideTgAngleL(const_cast<tTrkLocPos*>(&trkPos));
  //ci.yaw += M_PI / 2; // we use X axis as offset, Y as position in Golog
  NORM_PI_PI(ci.yaw);
  ci.pos = RtGetDistFromStart(car);
  ci.offset = trkPos.toMiddle;
  //ci.offset *= -1.0f; // we use X axis as offset, Y as position in Golog

  for (std::vector<cListener*>::const_iterator it = listeners.begin();
       it != listeners.end(); ++it) {
    cListener* listener = *it;
    if (times.find(car->index) == times.end() ||
        now - times[car->index] > listener->interval()) {
      listener->process(ci);
    }
  }
}

void cWorldModel::addListener(cWorldModel::cListener* listener)
{
  listeners.push_back(listener);
}


cWorldModel::cSimplePrologSerializor::cSimplePrologSerializor()
  : activated(false)
{
  mouseInfo = GfctrlMouseInit();
}

cWorldModel::cSimplePrologSerializor::~cSimplePrologSerializor()
{
  GfctrlMouseRelease(mouseInfo);
}

void cWorldModel::cSimplePrologSerializor::process(
    const cWorldModel::tCarInfo& ci)
{
  if (!activated) {
    GfctrlMouseGetCurrent(mouseInfo);
    if (mouseInfo->button[0] == 1 ||
        mouseInfo->edgedn[0] == 1 ||
        mouseInfo->edgeup[0] == 1) {
      ReMovieCapture();
      activated = true;
    }
  }

  if (!activated) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    int retval = select(1, &rfds, NULL, NULL, &tv);
    if (retval == -1) {
      perror("select()");
    } else if (retval) {
      activated = fgetc(stdin) != EOF;
    }
  }

  if (activated) {
#if 0
    if (!strcmp("Player", ci.name) && abs(rad2deg(ci.yaw)) >= 0.0)
      printf("observe(%2.5lf, deg('%s') = %2.2lf);\n",
             ci.time, ci.name, rad2deg(ci.yaw));
#else
    printf("obs(%lf, ["\
           "pos('%s') = %f, "\
           "offset('%s') = %f, "\
           "veloc('%s') = %f, "\
           "rad('%s') = %f, " \
           "deg('%s') = %f]).\n",
           ci.time,
           ci.name, ci.pos,
           ci.name, ci.offset,
           ci.name, ci.veloc,
           ci.name, ci.yaw,
           ci.name, rad2deg(ci.yaw));
#endif
  }
}

float cWorldModel::cSimplePrologSerializor::interval() const
{
  return 0.5f;
}

cWorldModel::cOffsetSerializor::cOffsetSerializor(const char *name)
  : fp((name) ? fopen(name, "wb") : NULL),
    row(0)
{
  if (fp) {
    const char *magic = "P6";
    const int depth = 255;
    char str[64];
    size_t len, written;

    sprintf(str, "%s\n", magic);
    len = strlen(str);
    written = fwrite(str, sizeof(char), len, fp);
    assert(written == len);

    sprintf(str, "%d %d\n", WIDTH, HEIGHT);
    len = strlen(str);
    written = fwrite(str, sizeof(char), len, fp);
    assert(written == len);

    sprintf(str, "%d\n", depth);
    len = strlen(str);
    written = fwrite(str, sizeof(char), len, fp);
    assert(written == len);

    buf = new unsigned char[3 * WIDTH];
    memset(buf, BG, 3 * WIDTH);
  }
}

cWorldModel::cOffsetSerializor::~cOffsetSerializor()
{
  if (fp) {
    fclose(fp);
    delete[] buf;
  }
}

void cWorldModel::cOffsetSerializor::process(
    const cWorldModel::tCarInfo& ci)
{
  if (fp) {
    if (!strcmp("Player", ci.name) || !strcmp("human", ci.name)) {
      if (mps2kmph(ci.veloc) > 50 && (++row) < HEIGHT) {
        const float offset = MIN(MAX_OFFSET, MAX(-1.0 * MAX_OFFSET, ci.offset));
        const int scaled_offset = static_cast<int>(offset * WIDTH / 2 / 10);
        const int pixel = WIDTH / 2 + scaled_offset;
        const size_t rowbytes = (size_t) WIDTH * 3;
        buf[pixel * 3 + 0] = FG;
        buf[pixel * 3 + 1] = FG;
        buf[pixel * 3 + 2] = FG;
        size_t written = fwrite(buf, sizeof(char), rowbytes, fp);
        assert(written == rowbytes);
        buf[pixel * 3 + 0] = BG;
        buf[pixel * 3 + 1] = BG;
        buf[pixel * 3 + 2] = BG;
      }
    }
  }
}

float cWorldModel::cOffsetSerializor::interval() const
{
  return 0.0f;
}

cWorldModel::cGraphicDisplay::cGraphicDisplay()
  : col(0)
{
}

cWorldModel::cGraphicDisplay::~cGraphicDisplay()
{
}

const float cWorldModel::cGraphicDisplay::WHITE[] = {1.0, 1.0, 1.0, 1.0};
const int cWorldModel::cGraphicDisplay::Y[] = {90, 70, 50, 30, 10};

void cWorldModel::cGraphicDisplay::process(
    const cWorldModel::tCarInfo& ci)
{
  float *white = const_cast<float*>(WHITE);
  int i;
  if (col == 0) {
    i = 0;
    GfuiPrintString("name", white, FONT, X, Y[i++], RIGHT_ALIGN);
    GfuiPrintString("pos", white, FONT, X, Y[i++], RIGHT_ALIGN);
    GfuiPrintString("offset", white, FONT, X, Y[i++], RIGHT_ALIGN);
    GfuiPrintString("veloc", white, FONT, X, Y[i++], RIGHT_ALIGN);
    GfuiPrintString("deg", white, FONT, X, Y[i++], RIGHT_ALIGN);
  }

  int x = X + (col + 1) * COLUMN_WIDTH;
  char buf[32];
  i = 0;
  GfuiPrintString(ci.name, white, FONT, x, Y[i++], LEFT_ALIGN);
  sprintf(buf, "%.0f", ci.pos);
  GfuiPrintString(buf, white, FONT, x, Y[i++], LEFT_ALIGN);
  sprintf(buf, "%.1f", ci.offset);
  GfuiPrintString(buf, white, FONT, x, Y[i++], LEFT_ALIGN);
  sprintf(buf, "%.1f", ci.veloc);
  GfuiPrintString(buf, white, FONT, x, Y[i++], LEFT_ALIGN);
  sprintf(buf, "%.1f", rad2deg(ci.yaw));
  GfuiPrintString(buf, white, FONT, x, Y[i++], LEFT_ALIGN);

  col = (col + 1) % 2;
}

float cWorldModel::cGraphicDisplay::interval() const
{
  return 0.0f;
}

