#include "worldmodel.h"

#include <sys/select.h>

#include <raceinit.h>
#include <tgfclient.h>

#include "macros.h"
#include "util.h"

int cWorldModel::priority() const {
  return 10000;
}

void cWorldModel::handle(cDriver& context)
{
  for (int i = 0; i < context.sit->_ncars; ++i) {
    const double now = context.sit->currentTime;
    /*
    tCarElt* car = context.sit->cars[i];
    if (times.find(car->index) == times.end() ||
        now - times[car->index] > 0.5) {
      fireEvents(now, context.sit->cars[i]);
      times[car->index] = now;
    }
    */
    fireEvents(context, now, context.sit->cars[i]);
  }
}

void cWorldModel::fireEvents(const cDriver& context, double now, tCarElt* car)
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

  for (size_t i = 0; i < listeners.size(); ++i) {
    cListener* listener = listeners[i];
    const std::pair<int, int> key = std::make_pair(car->index, i);
    const std::map<std::pair<int, int>, double>::const_iterator jt =
        times.find(key);
    if (jt == times.end() || now - jt->second > listener->interval()) {
      listener->process(context, ci);
      times[key] = now;
    }
  }
}

void cWorldModel::addListener(cWorldModel::cListener* listener)
{
  listeners.push_back(listener);
}


cWorldModel::cSimplePrologSerializor::cSimplePrologSerializor(const char *name)
  : fp(fopen_next(name, "ecl")),
    activated(false),
    virtualStart(-1.0),
    lastTime(0.0)
{
  mouseInfo = GfctrlMouseInit();
}

cWorldModel::cSimplePrologSerializor::~cSimplePrologSerializor()
{
  FILE *fps[] = { stdout, fp };
  for (size_t i = 0; i < sizeof(fps) / sizeof(*fps); ++i) {
    fprintf(fps[i], "\n%% EndOfRecord\n\n");
    fflush(fps[i]);
  }
  fclose(fp);
  GfctrlMouseRelease(mouseInfo);
}

void cWorldModel::cSimplePrologSerializor::process(
    const cDriver& context, const cWorldModel::tCarInfo& ci)
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

  activated = activated || mps2kmph(ci.veloc) > 50;

  if (activated) {
    FILE *fps[] = { stdout, fp };
    char nameTerm[32];

    sprintf(nameTerm, "'%s'", ci.name);

    /* Cars start before the starting line and therefore with a high position,
     * that is, the sequence of positions could be 2900, 3000, 3100, 100, 200.
     * These discontiguous don't fit our simple driving model in basic action
     * theory. Therefore, we choose a virtual starting line at the first
     * measured position minus some distance (the second measurement might be
     * even before the first measurement) and convert all subsequent
     * measurements into a position with respect the virtual starting line. */

    if (virtualStart < 0.0) {
      virtualStart = ci.pos - 250.0;
      assert(virtualStart >= 0.0);
    }

    double pos = ci.pos;
    if (pos >= virtualStart) {
      pos -= virtualStart;
    } else {
      pos += context.track->length - virtualStart;
    }

    for (size_t i = 0; i < sizeof(fps) / sizeof(*fps); ++i) {
      if (ci.time != lastTime) {
        fprintf(fps[i], "obs(%10.5lf, [", ci.time);
      } else {
        fprintf(fps[i], "                 ");
      }
      fprintf(fps[i],
              " pos(%10s) = %10.4f,"\
              " offset(%10s) = %7.3f,"\
              " veloc(%10s) = %10.6f,"\
              " rad(%10s) = %10.7f," \
              " deg(%10s) = %10.6f",
              nameTerm, pos,
              nameTerm, ci.offset,
              nameTerm, ci.veloc,
              nameTerm, ci.yaw,
              nameTerm, rad2deg(ci.yaw));
      if (ci.time != lastTime) {
        fprintf(fps[i], ",");
      } else {
        fprintf(fps[i], "]).\n");
      }
      fflush(fps[i]);
    }
    lastTime = ci.time;
  }
}

float cWorldModel::cSimplePrologSerializor::interval() const
{
  return 0.5f;
}

cWorldModel::cOffsetSerializor::cOffsetSerializor(const char *name)
  : img(name, WIDTH, HEIGHT),
    row(0)
{
  const int middle_of_the_street = WIDTH / 2;
  const int mark_diff = 15;
  const int mark_width = 4;
  bool line_mark = false;
  for (int row = 0; row < HEIGHT; ++row) {
    if (line_mark) {
      for (int i = -1 * mark_width / 2; i <= mark_diff / 2; ++i) {
        img.set_pixel(middle_of_the_street + i, row, tColor::GRAY);
      }
    }
    if (row % mark_diff == 0) {
      line_mark = !line_mark;
    }
  }
}

void cWorldModel::cOffsetSerializor::process(
    const cDriver& context, const cWorldModel::tCarInfo& ci)
{
  if (!strcmp("Player", ci.name) || !strcmp("human", ci.name)) {
    if (mps2kmph(ci.veloc) > 50 && row < HEIGHT) {
      const float offset = MIN(MAX_OFFSET, MAX(-1.0 * MAX_OFFSET, ci.offset));
      const int scaled_offset = static_cast<int>(offset * WIDTH / 2 / 10);
      const int pixel = WIDTH / 2 + scaled_offset;
      img.set_pixel(pixel, row, tColor::BLACK);
      ++row;
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
    const cDriver& context, const cWorldModel::tCarInfo& ci)
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

