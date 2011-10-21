#include "worldmodel.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/select.h>

#include <raceinit.h>
#include <raceengine.h>
#include <tgfclient.h>

#include "macros.h"
#include "util.h"

namespace colors {
const float WHITE[]  = {1.0, 1.0, 1.0, 1.0};
const float RED[]    = {1.0, 0.0, 0.0, 1.0};
const float GREEN[]  = {0.0, 1.0, 0.0, 1.0};
const float YELLOW[] = {1.0, 1.0, 0.0, 1.0};
}

namespace fonts {
const int F_SMALL = GFUI_FONT_LARGE_C;
const int F_LARGE = GFUI_FONT_LARGE_C;
const int F_BIG = GFUI_FONT_BIG_C;
const int F_HUGE = GFUI_FONT_HUGE_C;

const int F_SMALL_BOLD = GFUI_FONT_LARGE;
const int F_LARGE_BOLD = GFUI_FONT_LARGE;
const int F_BIG_BOLD = GFUI_FONT_BIG;
const int F_HUGE_BOLD = GFUI_FONT_HUGE;

const int ALIGN_CENTER = GFUI_ALIGN_HC_VC;
const int ALIGN_RIGHT = GFUI_ALIGN_HR_VC;
const int ALIGN_LEFT = GFUI_ALIGN_HL_VC;
}

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
      ReMovieCaptureHack(0);
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


class cWorldModel::cRedrawHookManager
{
 public:
  static cRedrawHookManager& instance() {
    return instance_;
  }

  static void redraw_hook() {
    instance_.redraw_all();
  }

  void register_handler(cRedrawable* handler)
  {
    handlers[nhandlers++] = handler;
    assert((size_t) nhandlers < sizeof(handlers) / sizeof(handlers[0]));
    if (nhandlers > 0) {
      ReSetRedrawHook(redraw_hook);
    }
  }

  void unregister_handler(cRedrawable* handler)
  {
    int i;
    for (i = 0; i < nhandlers; ++i) {
      if (handlers[i] == handler) {
        break;
      }
    }
    for (i = i + 1; i < nhandlers; ++i) {
      handlers[i-1] = handlers[i];
    }
    --nhandlers;
    if (nhandlers == 0) {
      ReSetRedrawHook(redraw_hook);
    }
  }

  void redraw_all()
  {
    for (int i = 0; i < nhandlers; ++i) {
      handlers[i]->redraw();
    }
  }

 private:
  static cRedrawHookManager instance_;

  cRedrawHookManager() : nhandlers(0) {
    ReSetRedrawHook(NULL);
  }

  DISALLOW_COPY_AND_ASSIGN(cRedrawHookManager);

  cRedrawable* handlers[10];
  int nhandlers;
};

cWorldModel::cRedrawHookManager cWorldModel::cRedrawHookManager::instance_;


cWorldModel::cGraphicInfoDisplay::cGraphicInfoDisplay()
  : init_phase(true),
    go_enabled(false),
    go_time(0.0l),
    wait_enabled(false)
{
  cRedrawHookManager::instance().register_handler(this);
}

cWorldModel::cGraphicInfoDisplay::~cGraphicInfoDisplay()
{
  cRedrawHookManager::instance().unregister_handler(this);
}

void cWorldModel::cGraphicInfoDisplay::process(
    const cDriver& context,
    const std::vector<tCarInfo>& infos)
{
  bool have_human = false;
  bool have_dummy = false;
  tCarInfo human = tCarInfo();
  tCarInfo dummy = tCarInfo();
  for (std::vector<tCarInfo>::const_iterator it = infos.begin();
       it != infos.end(); ++it) {
    map[it->name] = *it;

    if (have_dummy ||
        (!have_human && (!strcmp(it->name, "human") ||
                         !strcmp(it->name, "Player")))) {
      human = *it;
      have_human = true;
    } else {
      dummy = *it;
      have_dummy = true;
    }
  }

  const float GO_BLINK_TIME_ON = 0.5;
  const float GO_BLINK_TIME_OFF = 0.25;

  if (init_phase && have_human && have_dummy) {
    if (human.veloc <= kmph2mps(20.0) &&
        dummy.veloc >= 15.0) {
      const double now = dummy.time;
      const double period = go_enabled ? GO_BLINK_TIME_OFF : GO_BLINK_TIME_ON;
      if (now - go_time > period) {
        go_enabled = !go_enabled;
        go_time = now;
      }
    } else {
      go_enabled = false;
    }
    wait_enabled = go_time == 0.0l && dummy.veloc < 15.0;
    init_phase = human.veloc <= kmph2mps(20.0);
  }
}

void cWorldModel::cGraphicInfoDisplay::redraw()
{
  const int FONT = fonts::F_LARGE;
  const int X = 10;
  const int COLUMN_WIDTH = 75;
  const int Y[] = {140, 115, 90, 65, 40};

  float *white = const_cast<float*>(colors::WHITE);
  int i = 0;
  GfuiPrintString("name", white, FONT, X, Y[i++], fonts::ALIGN_LEFT);
  GfuiPrintString("pos", white, FONT, X, Y[i++], fonts::ALIGN_LEFT);
  GfuiPrintString("offset", white, FONT, X, Y[i++], fonts::ALIGN_LEFT);
  GfuiPrintString("veloc", white, FONT, X, Y[i++], fonts::ALIGN_LEFT);
  GfuiPrintString("deg", white, FONT, X, Y[i++], fonts::ALIGN_LEFT);

  int col = 0;
  for (std::map<std::string, tCarInfo>::const_iterator it = map.begin();
       it != map.end(); ++it)
  {
    const std::string& name = it->first;
    const tCarInfo& ci = it->second;
    int x = X + COLUMN_WIDTH + (col + 1) * COLUMN_WIDTH;
    char buf[32];
    int i = 0;
    GfuiPrintString(name.c_str(), white, FONT, x, Y[i++], fonts::ALIGN_RIGHT);
    sprintf(buf, "%.0f", ci.pos);
    GfuiPrintString(buf, white, FONT, x, Y[i++], fonts::ALIGN_RIGHT);
    sprintf(buf, "%.1f", ci.offset);
    GfuiPrintString(buf, white, FONT, x, Y[i++], fonts::ALIGN_RIGHT);
    sprintf(buf, "%.1f", ci.veloc);
    GfuiPrintString(buf, white, FONT, x, Y[i++], fonts::ALIGN_RIGHT);
    sprintf(buf, "%.1f", rad2deg(ci.yaw));
    GfuiPrintString(buf, white, FONT, x, Y[i++], fonts::ALIGN_RIGHT);
    ++col;

    if (wait_enabled || go_enabled) {
      float *yellow = const_cast<float*>(colors::YELLOW);
      float *green = const_cast<float*>(colors::GREEN);
      float *color = go_enabled ? green : yellow;
      const int x = 400;
      const int y = 400;
      const char* msg = go_enabled ? "Go get him!" : "Wait for it...";
      GfuiPrintString(msg, color, fonts::F_HUGE_BOLD,
                      x, y, fonts::ALIGN_CENTER);
    }
  }
}

float cWorldModel::cGraphicInfoDisplay::interval() const
{
  return 0.0f;
}


cWorldModel::cGraphicPlanRecogDisplay::cGraphicPlanRecogDisplay()
  : activated(false),
    offset(0)
{
  memset(buf, 0, sizeof(buf));
  cRedrawHookManager::instance().register_handler(this);
}

cWorldModel::cGraphicPlanRecogDisplay::~cGraphicPlanRecogDisplay()
{
  cRedrawHookManager::instance().unregister_handler(this);
}

bool cWorldModel::cGraphicPlanRecogDisplay::poll_str(char* buf, int& len)
{
  const int stdin_fd = 0;
  struct pollfd pin;
  const int timeout_msecs = 1;

  pin.fd = stdin_fd;
  pin.events = POLLIN | POLLRDHUP | POLLHUP;
  pin.revents = 0;

  int retval = ::poll(&pin, 1, timeout_msecs);

  if (retval < 0) {
    fprintf(stderr, "Can't poll, error %d\n", errno);
    return false;
  } else if (retval > 0 && (pin.revents & POLLIN) != 0) {
    len = (int) read(pin.fd, buf, (size_t) len);
    assert(len > 0);
    buf[len] = '\0';
    return true;
  } else {
    return false;
  }
}

bool cWorldModel::cGraphicPlanRecogDisplay::poll_line(char* buf, int& offset, int len)
{
  if (offset + 1 >= len) {
    fprintf(stderr, "cleaned buffer, might have ignored plan recog output\n");
    fflush(stderr);
    offset = 0;
  }

  char* suffix;
  buf[offset] = '\0';
  /* Drop all lines in the buffer. These were already read and
   * displayed. */
  if ((suffix = strrchr(buf, '\n')) != NULL) {
    const int drop_len = suffix - buf + 1;
    memmove(buf, buf + drop_len, len - drop_len);
    offset -= drop_len;
  }
  assert(strrchr(buf, '\n') == NULL);

  /* Fill buffer with new bytes. */
  int read_len = len - offset;
  if (poll_str(buf + offset, read_len)) {
    offset += read_len;
    return strchr(buf, '\n') != NULL;
  } else {
    return false;
  }
}

void cWorldModel::cGraphicPlanRecogDisplay::process(
    const cDriver& context, const tCarInfo& info)
{
  if (!activated) {
    activated = context.sit->currentTime > 5.0;
  }

  if (poll_line(buf, offset, sizeof(buf))) {
    /* One or more new line(s) is/are present. The next call to poll_line(),
     * even if it fails, drops the line from the string. */
    const char* suffix;
    if ((suffix = strrchr(buf, '\n')) != NULL) {
      for (char* str = buf; str != NULL; ) {
        while (*str == '\n') {
          ++str;
        }
        char* next = strchr(str + 1, '\n');
        if (next != NULL) {
          *next = '\0';
        }

        int succs;
        int total;
        double prob;
        if (sscanf(str, "%d / %d = %lf", &succs, &total, &prob) == 3) {
          if (activated) {
            last.succs = succs;
            last.total = total;
            last.prob = prob;
            if (prob >= best.prob) {
              best = last;
            }
          } else {
            fprintf(stderr, "ignoring plan recog result %d / %d = %lf, "\
                    "because it is probably from a previous run\n",
                    succs, total, prob);
            fflush(stderr);
          }
        }

        if (next != NULL) {
          *next = '\n';
          str = next + 1;
        } else {
          str = next;
        }
      }
    }
  }
}

void cWorldModel::cGraphicPlanRecogDisplay::redraw()
{
  print("last", 0, true, last);
  print("best", 1, false, best);
}

void cWorldModel::cGraphicPlanRecogDisplay::print(
    const char* label,
    int i,
    bool small,
    const cWorldModel::cGraphicPlanRecogDisplay::Result& r)
{
  float* const color = const_cast<float*>(r.prob > 0.02 ?
                                          colors::GREEN : colors::RED);
  const int font = small ? fonts::F_SMALL: fonts::F_BIG;
  const int x = 400;
  const int y = 550 - i * 30;
  char buf[128];
  sprintf(buf, "%s: %d / %d = %.1lf%%\n",
          label, r.succs, r.total, r.prob * 100);
  GfuiPrintString(buf, color, font, x, y, fonts::ALIGN_CENTER);
}

float cWorldModel::cGraphicPlanRecogDisplay::interval() const
{
  return 0.0f;
}

