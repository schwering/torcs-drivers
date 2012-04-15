#include "worldmodel.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/select.h>

#include <boost/bind.hpp>

#if 0
#include <planrecog.h>
#endif
#include <raceinit.h>
#include <raceengine.h>
#include <tgfclient.h>

#include "macros.h"
#include "util.h"

#include "../scenario.h"

#define MAXPROGLEN 512

namespace colors {
const float WHITE[]  = {1.0, 1.0, 1.0, 1.0};
const float RED[]    = {1.0, 0.0, 0.0, 1.0};
const float GREEN[]  = {0.0, 0.9, 0.0, 1.0};
const float YELLOW[] = {1.0, 1.0, 0.0, 1.0};
}

namespace fonts {
const int F_SMALL = GFUI_FONT_MEDIUM_C;
const int F_MEDIUM = GFUI_FONT_LARGE_C;
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


cWorldModel::cSimplePrologSerializor::cSimplePrologSerializor(const char *name)
  : fp(fopen_next(name, "ecl")),
    activated(false),
    virtualStart(-1.0)
{
}

cWorldModel::cSimplePrologSerializor::~cSimplePrologSerializor()
{
  ReMovieCaptureHack(0);
  FILE *fps[] = { stdout, fp };
  for (size_t i = 0; i < sizeof(fps) / sizeof(*fps); ++i) {
    if (fps[i]) {
      fprintf(fps[i], "\n%% EndOfRecord\n\n");
      fflush(fps[i]);
    }
  }
  fclose(fp);
  //GfctrlMouseRelease(mouseInfo);
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

  const bool prev_activated = activated;

  if (!prev_activated) {
    for (std::vector<tCarInfo>::const_iterator it = infos.begin();
         it != infos.end(); ++it) {
      activated = activated || mps2kmph(it->veloc) > 73;
#ifdef DA_ACCEL_LIMIT
      activated = activated || (!strcmp(it->name, "human") && mps2kmph(it->veloc) > 10);
#endif
    }
  }

  if (!prev_activated && activated) {
    fprintf(stderr, "Starting to observe.\n");
    ReMovieCaptureHack(0);
    FILE *fps[] = { stdout, fp };
    for (size_t i = 0; i < sizeof(fps) / sizeof(*fps); ++i) {
      if (fps[i]) {
        fprintf(fps[i], ":- module(obs).\n");
        fprintf(fps[i], ":- export(obs/2).\n");
        fprintf(fps[i], "\n");
        fprintf(fps[i], "%% BeginOfRecord\n\n");
        fflush(fps[i]);
      }
    }
  }

  if (activated) {
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

    FILE *fps[] = { stdout, fp };
    for (size_t i = 0; i < sizeof(fps) / sizeof(*fps); ++i) {
      FILE *fp = fps[i];
      if (!fp) {
        continue;
      }
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


cWorldModel::cSimpleMercurySerializor::cSimpleMercurySerializor(const char *name)
  : fp(fopen_next(name, "log")),
    activated(false),
    virtualStart(-1.0)
{
}

cWorldModel::cSimpleMercurySerializor::~cSimpleMercurySerializor()
{
  ReMovieCaptureHack(0);
  fclose(fp);
}

void cWorldModel::cSimpleMercurySerializor::process(
    const cDriver& context,
    const std::vector<tCarInfo>& infos)
{
  const bool prev_activated = activated;

  if (!prev_activated) {
    for (std::vector<tCarInfo>::const_iterator it = infos.begin();
         it != infos.end(); ++it) {
      activated = activated || mps2kmph(it->veloc) > 73;
#ifdef DA_ACCEL_LIMIT
      activated = activated || (!strcmp(it->name, "human") && mps2kmph(it->veloc) > 10);
#endif
    }
  }

  if (!prev_activated && activated) {
    fprintf(stderr, "Starting to observe.\n");
    ReMovieCaptureHack(0);
  }

  if (activated) {
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

    FILE *fps[] = { stdout, fp };
    for (size_t i = 0; i < sizeof(fps) / sizeof(*fps); ++i) {
      FILE *fp = fps[i];
      if (!fp) {
        continue;
      }
      for (std::vector<tCarInfo>::const_iterator it = infos.begin();
           it != infos.end(); ++it) {
        char kind = (!prev_activated && activated) ? 'I' : 'O';
        const char *name = (!strcmp(it->name, "human")) ? "b" : "a";
        double veloc = it->veloc;
        double yaw = it->yaw;
        double x = it->pos;
        if (x >= virtualStart) {
          x -= virtualStart;
        } else {
          x += context.track->length - virtualStart;
        }
        double y = it->offset;

        if (it == infos.begin()) {
          fprintf(fp, "%c\t%10.5lf\t", kind, it->time);
        }
        fprintf(fp, "%5s\t%10.5lf\t%10.5lf\t%10.5lf\t%10.5lf", name, veloc, yaw, x, y);
      }
      fprintf(fp, "\n");
      fflush(fp);
    }
  }
}

float cWorldModel::cSimpleMercurySerializor::interval() const
{
  return 0.5f;
}


#if 0
cWorldModel::cMercuryInterface::cMercuryInterface()
  : activated(false),
    virtualStart(-1.0)
{
  cRedrawHookManager::instance().register_handler(this);
  printf("%s:%d\n", __FILE__, __LINE__);
  mercury::initialize();
  printf("%s:%d\n", __FILE__, __LINE__);
  mercury::start_plan_recognition();
  printf("%s:%d\n", __FILE__, __LINE__);
}

cWorldModel::cMercuryInterface::~cMercuryInterface()
{
  cRedrawHookManager::instance().unregister_handler(this);
  ReMovieCaptureHack(0);
  mercury::finalize();
}

double cWorldModel::cMercuryInterface::normalize_pos(const cDriver& context,
                                                     double pos) const
{
  if (pos >= virtualStart) {
    pos -= virtualStart;
  } else {
    pos += context.track->length - virtualStart;
  }
  return pos;
}

void cWorldModel::cMercuryInterface::process(
    const cDriver& context,
    const std::vector<tCarInfo>& infos)
{
  if (infos.size() != 2) {
    return;
  }

  const bool prev_activated = activated;

  if (!prev_activated) {
    for (std::vector<tCarInfo>::const_iterator it = infos.begin();
         it != infos.end(); ++it) {
      activated = activated || mps2kmph(it->veloc) > 73;
#ifdef DA_ACCEL_LIMIT
      activated = activated || (!strcmp(it->name, "human") && mps2kmph(it->veloc) > 10);
#endif
    }
  }

  if (!prev_activated && activated) {
    fprintf(stderr, "Starting to observe.\n");
    ReMovieCaptureHack(0);
  }

  if (activated) {
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

    const tCarInfo& a0 = infos[0];
    const tCarInfo& a1 = infos[1];
    const char* a0_name = (!strcmp(a0.name, "human")) ? "b" : "a";
    const char* a1_name = (!strcmp(a1.name, "human")) ? "b" : "a";
    mercury::push_obs(a0.time,
                      a0_name, a0.veloc, a0.yaw,
                      normalize_pos(context, a0.pos), a0.offset,
                      a1_name, a1.veloc, a1.yaw,
                      normalize_pos(context, a1.pos), a1.offset);
  }
  printf("Confidence: %lf\n", mercury::confidence());
}

void cWorldModel::cMercuryInterface::redraw()
{
  print(0, false, colors::GREEN, "Mercury");
}

void cWorldModel::cMercuryInterface::print(int line,
                                           bool small,
                                           const float* color,
                                           const char* msg)
{
  const int font = small ? fonts::F_MEDIUM: fonts::F_BIG;
  const int x = 400;
  const int y = 550 - line * 45;
  GfuiPrintString(msg, const_cast<float*>(color), font, x, y,
                  fonts::ALIGN_CENTER);
}

float cWorldModel::cMercuryInterface::interval() const
{
  return 0.5f;
}
#endif


const char* cWorldModel::cMercuryClient::MERCURY_HOST = "localhost";
const char* cWorldModel::cMercuryClient::MERCURY_PORT = "19123";

cWorldModel::cMercuryClient::cMercuryClient()
  : work(io_service),
    io_thread(boost::bind(&boost::asio::io_service::run, &io_service)),
    socket(io_service),
    activated(false),
    virtualStart(-1.0)
{
  cRedrawHookManager::instance().register_handler(this);

  pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);
  using boost::asio::ip::tcp;
  tcp::resolver resolver(io_service);
  tcp::resolver::query query(tcp::v4(), MERCURY_HOST, MERCURY_PORT);
  tcp::resolver::iterator iterator = resolver.resolve(query);
  boost::asio::connect(socket, iterator);
  memset(&state_msg, 0, sizeof(&state_msg));
}

cWorldModel::cMercuryClient::~cMercuryClient()
{
  socket.close();
  io_service.stop();
  io_thread.join();
  pthread_spin_destroy(&spinlock);

  cRedrawHookManager::instance().unregister_handler(this);
  ReMovieCaptureHack(0);
}

double cWorldModel::cMercuryClient::normalize_pos(const cDriver& context,
                                                  double pos) const
{
  if (pos >= virtualStart) {
    pos -= virtualStart;
  } else {
    pos += context.track->length - virtualStart;
  }
  return pos;
}

void cWorldModel::cMercuryClient::process(
    const cDriver& context,
    const std::vector<tCarInfo>& infos)
{
  if (infos.size() != 2) {
    return;
  }

  const bool prev_activated = activated;

  if (!prev_activated) {
    for (std::vector<tCarInfo>::const_iterator it = infos.begin();
         it != infos.end(); ++it) {
      activated = activated || mps2kmph(it->veloc) > 73;
#ifdef DA_ACCEL_LIMIT
      activated = activated || (!strcmp(it->name, "human") && mps2kmph(it->veloc) > 10);
#endif
    }
  }

  if (!prev_activated && activated) {
    fprintf(stderr, "Starting to observe.\n");
    ReMovieCaptureHack(0);
  }

  if (activated) {
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

    const tCarInfo& a0 = infos[0];
    const tCarInfo& a1 = infos[1];

    struct record* rec = new struct record;
    rec->t = a0.time;

    strncpy(rec->agent0, (!strcmp(a0.name, "human")) ? "b" : "a", AGENTLEN);
    rec->veloc0 = a0.veloc;
    rec->rad0 = a0.yaw;
    rec->x0 = normalize_pos(context, a0.pos);
    rec->y0 = a0.offset;

    strncpy(rec->agent1, (!strcmp(a1.name, "human")) ? "b" : "a", AGENTLEN);
    rec->veloc1 = a1.veloc;
    rec->rad1 = a1.yaw;
    rec->x1 = normalize_pos(context, a1.pos);
    rec->y1 = a1.offset;

    //pthread_spin_lock(&owner->spinlock);
    boost::asio::async_write(
        socket, boost::asio::buffer(rec, sizeof(*rec)),
        boost::bind(&cMercuryClient::write_handler, this, rec,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));

    /*
    printf("%lf %s %lf %lf %lf %lf %s %lf %lf %lf %lf\n",
           rec->t,
           rec->agent0, rec->veloc0, rec->rad0, rec->x0, rec->y0,
           rec->agent1, rec->veloc1, rec->rad1, rec->x1, rec->y1);
    */
  }
}

void cWorldModel::cMercuryClient::write_handler(
    struct record* rec,
    const boost::system::error_code& ec,
    std::size_t bytes_transferred)
{
  delete rec;
  struct state_message* msg = new struct state_message;
  boost::asio::async_read(
      socket, boost::asio::buffer(msg, sizeof(*msg)),
      boost::bind(&cMercuryClient::read_handler, this, msg,
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void cWorldModel::cMercuryClient::read_handler(
    struct state_message* msg,
    const boost::system::error_code& ec,
    std::size_t bytes_transferred)
{
  memcpy(&state_msg, msg, sizeof(*msg));
  const float min_conf = min_confidence();
  const float max_conf = max_confidence();
  printf("%f =< Confidence =< %f\n", min_conf, max_conf);
  delete msg;
  pthread_spin_unlock(&spinlock);
}

float cWorldModel::cMercuryClient::min_confidence() const
{
  const int n = state_msg.working + state_msg.finished + state_msg.failed;
  return ((float) state_msg.finished) / ((float) n);
}

float cWorldModel::cMercuryClient::max_confidence() const
{
  const int n = state_msg.working + state_msg.finished + state_msg.failed;
  return ((float) state_msg.working + state_msg.finished) / ((float) n);
}

void cWorldModel::cMercuryClient::redraw()
{
  char min_buf[16];
  char max_buf[16];
  //pthread_spin_lock(&spinlock);
  const float min_conf = min_confidence();
  const float max_conf = max_confidence();
  //pthread_spin_unlock(&spinlock);
  sprintf(min_buf, "%.1f%%", min_conf * 100.0f);
  sprintf(max_buf, "%.1f%%", max_conf * 100.0f);
  const float* min_col = (min_conf > 0.02 ? colors::GREEN : colors::RED);
  const float* max_col = (max_conf > 0.02 ? colors::GREEN : colors::RED);
  const float* cen_col = (min_col == colors::GREEN ? colors::GREEN : max_col == colors::RED ? colors::RED : colors::YELLOW);
  print(1, -1, false, min_col, min_buf);
  print(1,  0, false, cen_col, "=< p =<");
  print(1,  1, false, max_col, max_buf);
}

void cWorldModel::cMercuryClient::print(int row,
                                        int col,
                                        bool small,
                                        const float* color,
                                        const char* msg)
{
  const int font = small ? fonts::F_MEDIUM: fonts::F_BIG;
  const int x = 400 + col * 150;
  const int y = 550 - row * 45;
  GfuiPrintString(msg, const_cast<float*>(color), font, x, y,
                  fonts::ALIGN_CENTER);
}

float cWorldModel::cMercuryClient::interval() const
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

    if (!have_human && (!strcmp(it->name, "human") ||
                         !strcmp(it->name, "Player"))) {
      human = *it;
      have_human = true;
    } else {
      dummy = (dummy.veloc > it->veloc) ? dummy : *it;
      have_dummy = true;
    }
  }

  const float GO_BLINK_TIME_ON = 0.5;
  const float GO_BLINK_TIME_OFF = 0.25;

  if (init_phase && have_human && have_dummy) {
    if (human.veloc <= kmph2mps(20.0) && dummy.veloc >= 15.0) {
      const double now = dummy.time;
      const double period = go_enabled ? GO_BLINK_TIME_OFF : GO_BLINK_TIME_ON;
      if (now - go_time > period) {
        go_enabled = !go_enabled;
        go_time = now;
      }
    } else {
      go_enabled = false;
    }
    wait_enabled = go_time == 0.0l &&
                   human.veloc <= kmph2mps(20.0) && dummy.veloc < 15.0;
    init_phase = human.veloc <= kmph2mps(20.0);
  }
}

#define NLINES 6
void cWorldModel::cGraphicInfoDisplay::redraw()
{
  const int FONT = fonts::F_SMALL;
  const int X = 10;
  const int COLUMN_WIDTH = 50;
  int Y[NLINES];

  Y[NLINES - 1] = 40;
  for (int i = NLINES - 2; i >= 0; --i) {
    Y[i] = Y[i+1] + 25;
  }

  float *white = const_cast<float*>(colors::YELLOW);
  int i = 0;
  GfuiPrintString("name", white, FONT, X, Y[i++], fonts::ALIGN_LEFT);
  GfuiPrintString("pos", white, FONT, X, Y[i++], fonts::ALIGN_LEFT);
  GfuiPrintString("offset", white, FONT, X, Y[i++], fonts::ALIGN_LEFT);
  GfuiPrintString("veloc", white, FONT, X, Y[i++], fonts::ALIGN_LEFT);
  GfuiPrintString("accel", white, FONT, X, Y[i++], fonts::ALIGN_LEFT);
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
    sprintf(buf, "%.1f", ci.accel);
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
    offset(0),
    n_updates(0)
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

bool cWorldModel::cGraphicPlanRecogDisplay::poll_line(char* buf,
                                                      int& offset,
                                                      int len)
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
    if (strrchr(buf, '\n') != NULL) {
      for (char* str = buf; str != NULL; ) {
        while (*str == '\n') {
          ++str;
        }
        char* next = strchr(str + 1, '\n');
        if (next != NULL) {
          *next = '\0';
        }

        char prog_name[MAXPROGLEN+1];
        int succs;
        int total;
        double prob;
        if (sscanf(str, "%s %d / %d = %lf",
                   prog_name, &succs, &total, &prob) == 4) {
          if (activated) {
            int len = strlen(prog_name);
            if (prog_name[len-1] == ':') {
              prog_name[len-1] = '\0';
            }
            Result& r = results[prog_name];
            ++n_updates;
            r.succs = succs;
            r.total = total;
            r.prob = prob;
          } else {
            fprintf(stderr, "ignoring plan recog result %d / %d = %lf"\
                    "for %s, because it is probably from a previous run\n",
                    succs, total, prob, prog_name);
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
  char buf[64];

  sprintf(buf, "#%d\n", n_updates);
  print(0, true, colors::YELLOW, buf);

  int index = 1;
  for (std::map<std::string, Result>::const_iterator it = results.begin();
       it != results.end(); ++it)
  {
    const std::string& s = it->first;
    const Result& r = it->second;
    if (r.succs != -1 || r.total != -1) {
      sprintf(buf, "%s: %d / %d = %.1lf%%\n",
              s.c_str(), r.succs, r.total, r.prob * 100);
      print(index, false, (r.prob > 0.02 ? colors::GREEN : colors::RED), buf);
      ++index;
    }
  }
}

void cWorldModel::cGraphicPlanRecogDisplay::print(int line,
                                                  bool small,
                                                  const float* color,
                                                  const char* msg)
{
  const int font = small ? fonts::F_MEDIUM: fonts::F_BIG;
  const int x = 400;
  const int y = 550 - line * 45;
  GfuiPrintString(msg, const_cast<float*>(color), font, x, y,
                  fonts::ALIGN_CENTER);
}

float cWorldModel::cGraphicPlanRecogDisplay::interval() const
{
  return 0.0f;
}

