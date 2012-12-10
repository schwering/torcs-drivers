#ifndef observerH
#define observerH

#include <map>
#include <string>
#include <vector>
#include <pthread.h>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <car-obs-torcs-types.h>

#include <tgfclient.h>

#include "driver.h"
#include "macros.h"
#include "pnmimage.h"
#include "wrapped_container.h"

class cObserver : public cDriver::cHandler
{
 public:
  struct tCarInfo
  {
    tCarInfo()
        : name(NULL), is_robot(true), time(0.0), veloc(0.0f), accel(0.0f),
          yaw(0.0f), pos(0.0f), offset(0.0f), laps(0) { }

    const char* name;
    bool is_robot;
    double time;
    float veloc;
    float accel;
    float yaw;
    float pos;
    float offset;
    int laps;
  };

  class cListener
  {
   public:
    cListener() : lastTime(0.0) {}
    virtual ~cListener() {}
    void process_or_skip(const cDriver& context,
                         const std::vector<tCarInfo>& infos);
    virtual void process(const cDriver& context,
                         const std::vector<tCarInfo>& infos);
    virtual void process(const cDriver& context,
                         const tCarInfo& info);
    virtual float interval() const = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(cListener);
    double lastTime;
  };

  class cRedrawable
  {
   public:
    virtual void redraw() = 0;
  };

  class cRedrawHookManager;

  class cSimpleMercurySerializor : public cListener
  {
   public:
    explicit cSimpleMercurySerializor(const char *name);
    virtual ~cSimpleMercurySerializor();
    virtual void process(const cDriver& context,
                         const std::vector<tCarInfo>& infos);
    virtual float interval() const;

   private:
    FILE *fp;
    bool activated;
    double virtualStart;
  };

  class cMercuryClient : public cListener, cRedrawable
  {
   public:
    explicit cMercuryClient();
    virtual ~cMercuryClient();
    double normalize_pos(const cDriver& context,
                         double pos) const;
    virtual void process(const cDriver& context,
                         const std::vector<tCarInfo>& infos);
    virtual float interval() const;
    virtual void redraw();

   private:
    static void* observation_thread(void* arg);

    static void print(int row,
                      int col,
                      bool small,
                      const float* color,
                      const char* msg);

    static const std::string MERCURY_HOST;
    static const std::string MERCURY_PORT;

    void write_handler(struct observation_record* obs_rec,
                       const boost::system::error_code& ec,
                       std::size_t bytes_transferred);

    void read_handler(struct planrecog_state* msg,
                      const boost::system::error_code& ec,
                      std::size_t bytes_transferred);

    float min_confidence(int source) const;
    float max_confidence(int source) const;
    std::vector< std::pair<float, float> > confidences() const;

    pthread_spinlock_t spinlock;
    boost::asio::io_service io_service;
    boost::asio::io_service::work work;
    boost::thread io_thread;
    boost::asio::ip::tcp::socket socket;
    bool activated;
    double virtualStart;
    struct planrecog_state planrecog_state;
  };

  class cGraphicInfoDisplay : public cListener, cRedrawable
  {
   public:
    cGraphicInfoDisplay();
    virtual ~cGraphicInfoDisplay();
    virtual void process(const cDriver& context,
                         const std::vector<tCarInfo>& infos);
    virtual float interval() const;
    virtual void redraw();

   private:
    std::map<std::string, tCarInfo> map;
    bool init_phase;
    bool go_enabled;
    double go_time;
    bool wait_enabled;
  };

#if 0
  class cGraphicPlanRecogDisplay : public cListener, cRedrawable
  {
   public:
    cGraphicPlanRecogDisplay();
    virtual ~cGraphicPlanRecogDisplay();
    virtual void process(const cDriver& context,
                         const tCarInfo& infos);
    virtual float interval() const;
    virtual void redraw();

   private:
    struct Result {
      Result() : n(0), succs(-1), total(-1), prob(0.0) { }
      Result(const Result& r) : succs(r.succs), total(r.total), prob(r.prob) { }

      int n;
      int succs;
      int total;
      double prob;
    };

    /* Returns true iff new data is present. In this case, at most len bytes
     * are copied into the buffer. */
    static bool poll_str(char* buf, int& len);
    /* First cleans any complete lines from buf and then calls poll_str() and
     * returns true if a complete line is in the buffer. */
    static bool poll_line(char* buf, int& offset, int len);

    static void print(int line,
                      bool small,
                      const float* color,
                      const char* msg);

    bool activated;
    char buf[4096];
    int offset;
    int n_updates;
    std::map<std::string, Result> results;
  };
#endif

  virtual ~cObserver() {}

  virtual int priority() const;
  virtual void handle(cDriver& context);

  void addListener(cListener* listener);

 private:
  static tCarInfo build_car_info(double now, tCarElt* car);

  wrapped_container< std::vector<cListener*> > listeners;
};

#endif

