#include "observer.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/select.h>

#include <boost/bind.hpp>

#include <raceinit.h>
#include <tgfclient.h>
#include <raceengine.h>

#include "macros.h"
#include "util.h"

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

const int F_SMALL_BOLD = GFUI_FONT_LARGE;
const int F_LARGE_BOLD = GFUI_FONT_LARGE;
const int F_BIG_BOLD = GFUI_FONT_BIG;

const int ALIGN_CENTER = GFUI_ALIGN_HC_VC;
const int ALIGN_RIGHT = GFUI_ALIGN_HR_VC;
const int ALIGN_LEFT = GFUI_ALIGN_HL_VC;
}

int cObserver::priority() const {
  return 10000;
}

void cObserver::handle(cDriver& context)
{
  const double now = context.sit->currentTime;
  const int ncars = context.sit->_ncars;
  if (ncars == 0) {
    return;
  }
  std::vector<tCarInfo> infos(ncars - 1);
  for (int i = 0, j = 0; i < ncars; ++i) {
    if (context.sit->cars[i] != context.car) {
      assert(j < ncars - 1);
      infos[j++] = build_car_info(now, context.sit->cars[i]);
    }
  }
  for (size_t i = 0; i < listeners.size(); ++i) {
    listeners[i]->process_or_skip(context, infos);
  }
}

cObserver::tCarInfo cObserver::build_car_info(double now, tCarElt* car)
{
  tCarInfo ci;
  ci.name = car->_name;
  ci.is_robot = (car->_driverType == RM_DRV_ROBOT);
  assert(car->_driverType == RM_DRV_ROBOT || car->_driverType == RM_DRV_HUMAN);
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

void cObserver::addListener(cObserver::cListener* listener)
{
  listeners.push_back(listener);
}

void cObserver::cListener::process_or_skip(
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

void cObserver::cListener::process(
    const cDriver& context,
    const std::vector<tCarInfo>& infos)
{
  /* default impl forwards */
  for (std::vector<tCarInfo>::const_iterator it = infos.begin();
       it != infos.end(); ++it) {
    process(context, *it);
  }
}

void cObserver::cListener::process(
    const cDriver& context,
    const tCarInfo& info)
{
  /* dummy */
}


class cObserver::cRedrawHookManager
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


cObserver::cSimpleMercurySerializor::cSimpleMercurySerializor(const char *name)
  : fp(fopen_next(name, "log")),
    activated(false),
    activationTime(-1.0),
    virtualStart(-1.0)
{
}

cObserver::cSimpleMercurySerializor::~cSimpleMercurySerializor()
{
  fclose(fp);
}

void cObserver::cSimpleMercurySerializor::process(
    const cDriver& context,
    const std::vector<tCarInfo>& infos)
{
  const bool prev_activated = activated;

  if (!prev_activated) {
    for (std::vector<tCarInfo>::const_iterator it = infos.begin();
         it != infos.end(); ++it) {
#if 0
      activated = activated || (!it->is_robot && mps2kmph(it->veloc) > 60.0f);
#else
      activated = activated || (!it->is_robot && mps2kmph(it->veloc) > 10);
#endif
    }
    if (activated) {
      activationTime = context.sit->currentTime;
    }
  }

  if (!prev_activated && activated) {
    fprintf(stderr, "Starting to observe.\n");
  }

  if (activated && context.sit->currentTime > activationTime + 1.0) {
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
        const char *name = it->name;
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

float cObserver::cSimpleMercurySerializor::interval() const
{
  return 0.5f;
}


template<class T>
std::string t_to_string(T i)
{
  std::stringstream ss;
  std::string s;
  ss << i;
  s = ss.str();
  return s;
}


const std::string cObserver::cMercuryClient::MERCURY_HOST = "localhost";
const std::string cObserver::cMercuryClient::MERCURY_PORT = t_to_string(PORT);

cObserver::cMercuryClient::cMercuryClient()
  : work(io_service),
    io_thread(boost::bind(&boost::asio::io_service::run, &io_service)),
    socket(io_service),
    activated(false),
    activationTime(-1.0),
    virtualStart(-1.0)
{
  cRedrawHookManager::instance().register_handler(this);

  pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);
  memset(&planrecog_state, 0, sizeof(&planrecog_state));
}

void cObserver::cMercuryClient::connect()
{
  using boost::asio::ip::tcp;
  tcp::resolver resolver(io_service);
  tcp::resolver::query query(tcp::v4(), MERCURY_HOST, MERCURY_PORT);
  tcp::resolver::iterator iterator = resolver.resolve(query);
  try {
    boost::asio::connect(socket, iterator);
  } catch (const std::exception& exc) {
    fprintf(stderr, "Connecting to Mercury server %s:%s failed.\n",
            MERCURY_HOST.c_str(), MERCURY_PORT.c_str());
    throw;
  }
}

cObserver::cMercuryClient::~cMercuryClient()
{
  socket.close();
  io_service.stop();
  io_thread.join();
  pthread_spin_destroy(&spinlock);

  cRedrawHookManager::instance().unregister_handler(this);
}

namespace {
double normalize_pos(double virtualStart,
                     const cDriver& context,
                     double pos)
{
  if (pos >= virtualStart) {
    pos -= virtualStart;
  } else {
    pos += context.track->length - virtualStart;
  }
  return pos;
}
}

double cObserver::cMercuryClient::normalize_pos(const cDriver& context,
                                                double pos) const
{
  return ::normalize_pos(virtualStart, context, pos);
}

void cObserver::cMercuryClient::process(
    const cDriver& context,
    const std::vector<tCarInfo>& infos)
{
  const bool prev_activated = activated;

  if (!prev_activated) {
    for (std::vector<tCarInfo>::const_iterator it = infos.begin();
         it != infos.end(); ++it) {
#if 0
      activated = activated || mps2kmph(it->veloc) > 75.0f;
#else
      activated = activated || (!it->is_robot && mps2kmph(it->veloc) > 10.0f);
#endif
    }
    if (activated) {
      activationTime = context.sit->currentTime;
    }
  }

  if (!prev_activated && activated) {
    fprintf(stderr, "Starting to observe.\n");
  }

  if (activated && context.sit->currentTime > activationTime + 1.0) {
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

    struct observation_record* obs_rec = new struct observation_record;
    memset(obs_rec, 0, sizeof(*obs_rec));
    obs_rec->t = infos[0].time;
    obs_rec->n_agents = infos.size();

    assert(infos.size() > 1);
    assert(infos.size() <= NAGENTS);
    for (std::vector<tCarInfo>::const_iterator it = infos.begin();
         it != infos.end(); ++it) {
      const int i = agent_to_index(it->name);
      assert(0 <= i && i < NAGENTS);
      obs_rec->info[i].present = 1;
      strncpy(obs_rec->info[i].agent, it->name, AGENTLEN);
      obs_rec->info[i].veloc = it->veloc;
      obs_rec->info[i].rad = it->yaw;
      obs_rec->info[i].x = normalize_pos(context, it->pos);
      obs_rec->info[i].y = it->offset;
    }

    //pthread_spin_lock(&owner->spinlock);
    boost::asio::async_write(
        socket, boost::asio::buffer(obs_rec, sizeof(*obs_rec)),
        boost::bind(&cMercuryClient::write_handler, this, obs_rec,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));

    /*
    printf("%lf %s %lf %lf %lf %lf %s %lf %lf %lf %lf\n",
           obs_rec->t,
           obs_rec->agent0, obs_rec->veloc0, obs_rec->rad0, obs_rec->x0, obs_rec->y0,
           obs_rec->agent1, obs_rec->veloc1, obs_rec->rad1, obs_rec->x1, obs_rec->y1);
    */
  }
}

void cObserver::cMercuryClient::write_handler(
    struct observation_record* obs_rec,
    const boost::system::error_code& ec,
    std::size_t bytes_transferred)
{
  delete obs_rec;
  struct planrecog_state* msg = new struct planrecog_state;
  boost::asio::async_read(
      socket, boost::asio::buffer(msg, sizeof(*msg)),
      boost::bind(&cMercuryClient::read_handler, this, msg,
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void cObserver::cMercuryClient::read_handler(
    struct planrecog_state* msg,
    const boost::system::error_code& ec,
    std::size_t bytes_transferred)
{
  memcpy(&planrecog_state, msg, sizeof(*msg));
  /*
  std::vector< std::pair<float, float> > cs = confidences();
  for (size_t i = 0; i < cs.size(); ++i)
  {
    const float min_conf = cs[i].first;
    const float max_conf = cs[i].second;
    printf("%lu: %f =< Confidence =< %f\n", i, min_conf, max_conf);
  }
  */
  delete msg;
  pthread_spin_unlock(&spinlock);
}

float cObserver::cMercuryClient::min_confidence(int source) const
{
  const int numer = planrecog_state.sources[source].finished;
  const int denom = planrecog_state.sources[source].working +
                    planrecog_state.sources[source].finished +
                    planrecog_state.sources[source].failed;
  return ((float) numer) / ((float) denom);
}

float cObserver::cMercuryClient::max_confidence(int source) const
{
  const int numer = planrecog_state.sources[source].working +
                    planrecog_state.sources[source].finished;
  const int denom = planrecog_state.sources[source].working +
                    planrecog_state.sources[source].finished +
                    planrecog_state.sources[source].failed;
  return ((float) numer) / ((float) denom);
}

std::vector< std::pair<float, float> >
cObserver::cMercuryClient::confidences() const
{
  std::vector< std::pair<float, float> > v(planrecog_state.n_sources);
  for (int i = 0; i < planrecog_state.n_sources; ++i) {
    v[i] = std::make_pair(min_confidence(i), max_confidence(i));
  }
  return v;
}

void cObserver::cMercuryClient::redraw()
{
  //pthread_spin_lock(&spinlock);
  std::vector< std::pair<float, float> > cs = confidences();
  //pthread_spin_unlock(&spinlock);
  for (size_t i = 0; i < cs.size(); ++i)
  {
    const float min_conf = cs[i].first;
    const float max_conf = cs[i].second;
    char min_buf[16];
    char max_buf[16];
    char cen_buf[16];
    sprintf(min_buf, "%.1f%%", min_conf * 100.0f);
    sprintf(max_buf, "%.1f%%", max_conf * 100.0f);
    sprintf(cen_buf, "=< p_%lu =<", i);
    const float* min_col = (min_conf > 0.02 ? colors::GREEN : colors::RED);
    const float* max_col = (max_conf > 0.02 ? colors::GREEN : colors::RED);
    const float* cen_col = (min_col == colors::GREEN ? colors::GREEN : max_col == colors::RED ? colors::RED : colors::YELLOW);
    print(i+1, -1, false, min_col, min_buf);
    print(i+1,  0, false, cen_col, cen_buf);
    print(i+1,  1, false, max_col, max_buf);
  }
}

void cObserver::cMercuryClient::print(int row,
                                      int col,
                                      bool small,
                                      const float* color,
                                      const char* msg)
{
  const int font = small ? fonts::F_MEDIUM: fonts::F_BIG;
  const int x = 400 + col * 180;
  const int y = 550 - row * 45;
  GfuiPrintString(msg, const_cast<float*>(color), font, x, y,
                  fonts::ALIGN_CENTER);
}

float cObserver::cMercuryClient::interval() const
{
  return 0.5f;
}


cObserver::cRedrawHookManager cObserver::cRedrawHookManager::instance_;


cObserver::cGraphicInfoDisplay::cGraphicInfoDisplay()
  : init_phase(true),
    go_enabled(false),
    go_time(0.0l),
    wait_enabled(false),
    virtualStart(-1.0)
{
  cRedrawHookManager::instance().register_handler(this);
}

cObserver::cGraphicInfoDisplay::~cGraphicInfoDisplay()
{
  cRedrawHookManager::instance().unregister_handler(this);
}

void cObserver::cGraphicInfoDisplay::process(
    const cDriver& context,
    const std::vector<tCarInfo>& infos)
{
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

  bool found_human = false;
  bool found_dummy = false;
  tCarInfo human = tCarInfo();
  tCarInfo dummy = tCarInfo();
  for (std::vector<tCarInfo>::const_iterator it = infos.begin();
       it != infos.end(); ++it) {
    tCarInfo copy = *it;
    copy.pos = ::normalize_pos(virtualStart, context, it->pos);
    map[it->name] = copy;

    if (!found_human && !it->is_robot) {
      human = *it;
      found_human = true;
    } else {
      dummy = (dummy.veloc > it->veloc) ? dummy : *it;
      found_dummy = true;
    }
  }

  const float GO_BLINK_TIME_ON = 0.5;
  const float GO_BLINK_TIME_OFF = 0.25;

  if (init_phase && found_human && found_dummy) {
    if (human.veloc <= kmph2mps(70.0) && dummy.veloc >= kmph2mps(50.0)) {
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
                   human.veloc <= kmph2mps(70.0) && dummy.veloc < kmph2mps(50.0);
    init_phase = human.veloc <= kmph2mps(70.0);
  }
}

void cObserver::cGraphicInfoDisplay::redraw()
{
  draw_info_sheet();
  draw_distance_sheet();
  draw_message();
}

void cObserver::cGraphicInfoDisplay::draw_info_sheet()
{
  const int FONT = fonts::F_SMALL;
  const int X = 10;
  const int COLUMN_WIDTH = 50;
  const int NLINES = 6;
  int Y[NLINES];

  Y[NLINES - 1] = 40;
  for (int i = NLINES - 2; i >= 0; --i) {
    Y[i] = Y[i+1] + 25;
  }

  float *yellow = const_cast<float*>(colors::YELLOW);
  int i = 0;
  GfuiPrintString("name", yellow, FONT, X, Y[i++], fonts::ALIGN_LEFT);
  GfuiPrintString("pos", yellow, FONT, X, Y[i++], fonts::ALIGN_LEFT);
  GfuiPrintString("offset", yellow, FONT, X, Y[i++], fonts::ALIGN_LEFT);
  GfuiPrintString("veloc", yellow, FONT, X, Y[i++], fonts::ALIGN_LEFT);
  GfuiPrintString("accel", yellow, FONT, X, Y[i++], fonts::ALIGN_LEFT);
  GfuiPrintString("deg", yellow, FONT, X, Y[i++], fonts::ALIGN_LEFT);

  int col = 0;
  for (std::map<std::string, tCarInfo>::const_iterator it = map.begin();
       it != map.end(); ++it)
  {
    const std::string& name = it->first;
    const tCarInfo& ci = it->second;
    int x = X + COLUMN_WIDTH + (col + 1) * COLUMN_WIDTH;
    char buf[32];
    int i = 0;
    GfuiPrintString(name.c_str(), yellow, FONT, x, Y[i++], fonts::ALIGN_RIGHT);
    sprintf(buf, "%.0f", ci.pos);
    GfuiPrintString(buf, yellow, FONT, x, Y[i++], fonts::ALIGN_RIGHT);
    sprintf(buf, "%.1f", ci.offset);
    GfuiPrintString(buf, yellow, FONT, x, Y[i++], fonts::ALIGN_RIGHT);
    sprintf(buf, "%.1f", ci.veloc);
    GfuiPrintString(buf, yellow, FONT, x, Y[i++], fonts::ALIGN_RIGHT);
    sprintf(buf, "%.1f", ci.accel);
    GfuiPrintString(buf, yellow, FONT, x, Y[i++], fonts::ALIGN_RIGHT);
    sprintf(buf, "%.1f", rad2deg(ci.yaw));
    GfuiPrintString(buf, yellow, FONT, x, Y[i++], fonts::ALIGN_RIGHT);
    ++col;
  }
}

void cObserver::cGraphicInfoDisplay::draw_distance_sheet()
{
  const int FONT = fonts::F_MEDIUM;
  const int X = 790;
  const int DELTA_Y = 25;
  float *yellow = const_cast<float*>(colors::YELLOW);

  int y = 250;
  for (std::map<std::string, tCarInfo>::const_iterator it = map.begin();
       it != map.end(); ++it)
  {
    const std::string& bn = it->first;
    const tCarInfo& bi = it->second;
    for (std::map<std::string, tCarInfo>::const_iterator jt = map.begin();
         jt != map.end(); ++jt)
    {
      if (it == jt)
        continue;
      char ntg_buf[64];
      char ttc_buf[64];
      const std::string& cn = jt->first;
      const tCarInfo& ci = jt->second;
      const double dist = ci.pos - bi.pos;
      const float ntg = dist / bi.veloc;
      const float ttc = dist / (bi.veloc - ci.veloc);
      sprintf(ntg_buf, "ntg(%s,%s) = %.1f", bn.c_str(), cn.c_str(), ntg);
      sprintf(ttc_buf, "ttc(%s,%s) = %.1f", bn.c_str(), cn.c_str(), ttc);
      y -= DELTA_Y;
      GfuiPrintString(ntg_buf, yellow, FONT, X, y, fonts::ALIGN_RIGHT);
      y -= DELTA_Y;
      GfuiPrintString(ttc_buf, yellow, FONT, X, y, fonts::ALIGN_RIGHT);
    }
  }
}

void cObserver::cGraphicInfoDisplay::draw_message()
{
  for (std::map<std::string, tCarInfo>::const_iterator it = map.begin();
       it != map.end(); ++it)
  {
    if (wait_enabled || go_enabled) {
      float *yellow = const_cast<float*>(colors::YELLOW);
      float *green = const_cast<float*>(colors::GREEN);
      float *color = go_enabled ? green : yellow;
      const int x = 400;
      const int y = 400;
      const char* msg = go_enabled ? "Go get him!" : "Wait for it...";
      GfuiPrintString(msg, color, fonts::F_BIG_BOLD,
                      x, y, fonts::ALIGN_CENTER);
    }
  }

}

float cObserver::cGraphicInfoDisplay::interval() const
{
  return 0.0f;
}


#if 0
cObserver::cGraphicPlanRecogDisplay::cGraphicPlanRecogDisplay()
  : activated(false),
    offset(0),
    n_updates(0)
{
  memset(buf, 0, sizeof(buf));
  cRedrawHookManager::instance().register_handler(this);
}

cObserver::cGraphicPlanRecogDisplay::~cGraphicPlanRecogDisplay()
{
  cRedrawHookManager::instance().unregister_handler(this);
}

bool cObserver::cGraphicPlanRecogDisplay::poll_str(char* buf, int& len)
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

bool cObserver::cGraphicPlanRecogDisplay::poll_line(char* buf,
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

void cObserver::cGraphicPlanRecogDisplay::process(
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

void cObserver::cGraphicPlanRecogDisplay::redraw()
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

void cObserver::cGraphicPlanRecogDisplay::print(int line,
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

float cObserver::cGraphicPlanRecogDisplay::interval() const
{
  return 0.0f;
}
#endif

