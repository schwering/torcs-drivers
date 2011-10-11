#include "worldmodel.h"

#include <sys/select.h>

#include <raceinit.h>
#include <raceengine.h>
#include <tgfclient.h>

#include "macros.h"
#include "util.h"

int cWorldModel::priority() const {
  return 10000;
}

void cWorldModel::handle(cDriver& context)
{
  const double now = context.sit->currentTime;
  const int ncars = context.sit->_ncars;
  if (ncars == 0) {
    return;
  }
  std::vector<tCarInfo> infos(ncars);

  for (int i = 0; i < ncars; ++i) {
    infos[i] = build_car_info(now, context.sit->cars[i]);
  }

  for (size_t i = 0; i < listeners.size(); ++i) {
    listeners[i]->process_or_skip(context, infos);
  }
}

cWorldModel::tCarInfo cWorldModel::build_car_info(double now, tCarElt* car)
{
  tCarInfo ci;
  ci.name = car->_name;
  ci.time = now;
  ci.veloc = car->_speed_x;
  ci.accel = car->_accel_x;
  ci.laps = car->_laps;

  const tTrkLocPos trkPos = car->_trkPos;
  ci.yaw = car->_yaw - RtTrackSideTgAngleL(const_cast<tTrkLocPos*>(&trkPos));
  //ci.yaw += M_PI / 2; // we use X axis as offset, Y as position in Golog
  NORM_PI_PI(ci.yaw);

  ci.pos = RtGetDistFromStart(car);
  ci.offset = trkPos.toMiddle;
  //ci.offset *= -1.0f; // we use X axis as offset, Y as position in Golog

  return ci;
}

void cWorldModel::addListener(cWorldModel::cListener* listener)
{
  listeners.push_back(listener);
}


void cWorldModel::cListener::process_or_skip(
    const cDriver& context,
    const std::vector<tCarInfo>& infos)
{
  assert(infos.size() > 0);
  const double now = infos[0].time;
  if (now - lastTime >= interval()) {
    process(context, infos);
    lastTime = now;
  }
}


void cWorldModel::cListener::process(
    const cDriver& context,
    const std::vector<tCarInfo>& infos)
{
  /* default impl forwards */
  for (std::vector<tCarInfo>::const_iterator it = infos.begin();
       it != infos.end(); ++it) {
    process(context, *it);
  }
}


void cWorldModel::cListener::process(
    const cDriver& context,
    const tCarInfo& info)
{
  /* dummy */
}


cWorldModel::cSimplePrologSerializor::cSimplePrologSerializor(const char *name)
  : fp(fopen_next(name, "ecl")),
    activated(false),
    virtualStart(-1.0)
{
  mouseInfo = GfctrlMouseInit();
}

cWorldModel::cSimplePrologSerializor::~cSimplePrologSerializor()
{
  ReMovieCaptureHack(0);
  FILE *fps[] = { stdout, fp };
  for (size_t i = 0; i < sizeof(fps) / sizeof(*fps); ++i) {
    fprintf(fps[i], "\n%% EndOfRecord\n\n");
    fflush(fps[i]);
  }
  fclose(fp);
  GfctrlMouseRelease(mouseInfo);
}

void cWorldModel::cSimplePrologSerializor::process(
    const cDriver& context,
    const std::vector<tCarInfo>& infos)
{
  /*
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
  */

  bool prev_activated = activated;

  for (std::vector<tCarInfo>::const_iterator it = infos.begin();
       it != infos.end(); ++it) {
    activated = activated || mps2kmph(it->veloc) > 70;
  }

  if (!prev_activated && activated) {
      ReMovieCaptureHack(10);
      FILE *fps[] = { stdout, fp };
      for (size_t i = 0; i < sizeof(fps) / sizeof(*fps); ++i) {
        fprintf(fps[i], ":- module(obs).\n");
        fprintf(fps[i], ":- export(obs/2).\n");
        fprintf(fps[i], "\n");
        fprintf(fps[i], "%% BeginOfRecord\n\n");
        fflush(fps[i]);
      }
  }

  if (activated) {
    FILE *fps[] = { stdout, fp };

    /* Cars start before the starting line and therefore with a high position,
     * that is, the sequence of positions could be 2900, 3000, 3100, 100, 200.
     * These discontiguous don't fit our simple driving model in basic action
     * theory. Therefore, we choose a virtual starting line at the minimal first
     * measured position minus. */

    if (virtualStart < 0.0) {
      bool init = false;
      double minPos = 0.0;
      int minLaps = 0;
      for (std::vector<tCarInfo>::const_iterator it = infos.begin();
           it != infos.end(); ++it) {
        if (!init ||
            it->laps < minLaps ||
            (it->laps == minLaps && it->pos < minPos)) {
          init = true;
          minPos = it->pos;
          minLaps = it->laps;
        }
      }
      virtualStart = minPos;
      assert(init);
      assert(virtualStart >= 0.0);
    }

    for (size_t i = 0; i < sizeof(fps) / sizeof(*fps); ++i) {
      FILE *fp = fps[i];
      for (std::vector<tCarInfo>::const_iterator it = infos.begin();
           it != infos.end(); ++it) {
        char nameTerm[32];
        sprintf(nameTerm, "'%s'", it->name);

        double pos = it->pos;
        if (pos >= virtualStart) {
          pos -= virtualStart;
        } else {
          pos += context.track->length - virtualStart;
        }

        if (it == infos.begin()) {
          fprintf(fp, "obs(%10.5lf, [", it->time);
        } else {
          fprintf(fp, ",");
          fprintf(fp, "                 ");
        }
        fprintf(fp,
                " pos(%10s) = %10.4f,"\
                " offset(%10s) = %7.3f,"\
                " veloc(%10s) = %10.6f,"\
                " rad(%10s) = %10.7f," \
                " deg(%10s) = %10.6f",
                nameTerm, pos,
                nameTerm, it->offset,
                nameTerm, it->veloc,
                nameTerm, it->yaw,
                nameTerm, rad2deg(it->yaw));
      }
      fprintf(fp, "]).\n");
      fflush(fp);
    }
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

