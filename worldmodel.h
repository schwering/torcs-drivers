#ifndef worldmodelH
#define worldmodelH

#include <map>
#include <vector>

#include <tgfclient.h>

#include "driver.h"
#include "macros.h"
#include "pnmimage.h"
#include "wrapped_container.h"

class cWorldModel : public cDriver::cHandler
{
 public:
  struct tCarInfo
  {
    const char* name;
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

  class cSimplePrologSerializor : public cListener
  {
   public:
    explicit cSimplePrologSerializor(const char *name);
    virtual ~cSimplePrologSerializor();
    virtual void process(const cDriver& context,
                         const std::vector<tCarInfo>& infos);
    virtual float interval() const;

   private:
    FILE *fp;
    bool activated;
    tCtrlMouseInfo* mouseInfo;
    double virtualStart;
  };

  class cOffsetSerializor : public cListener
  {
   public:
    explicit cOffsetSerializor(const char *name);
    virtual void process(const cDriver& context,
                         const tCarInfo& info);
    virtual float interval() const;

   private:
    static const int WIDTH = 1024;
    static const int HEIGHT = 4096;
    static const int MAX_OFFSET = 7;
    static const unsigned char BG = 255;
    static const unsigned char FG = 0;

    cPnmImage img;
    int row;
  };

  class cGraphicInfoDisplay : public cListener
  {
   public:
    cGraphicInfoDisplay();
    virtual ~cGraphicInfoDisplay();
    virtual void process(const cDriver& context,
                         const tCarInfo& info);
    virtual float interval() const;
   private:
    static const int FONT = GFUI_FONT_SMALL_C;
    static const int RIGHT_ALIGN = GFUI_ALIGN_HL_VT;
    static const int LEFT_ALIGN = GFUI_ALIGN_HL_VC;
    static const int X = 80;
    static const int COLUMN_WIDTH = 50;
    static const int Y[5];
    int col;
  };

  class cGraphicPlanRecogDisplay : public cListener
  {
   public:
    cGraphicPlanRecogDisplay();
    virtual ~cGraphicPlanRecogDisplay();
    virtual void process(const cDriver& context,
                         const tCarInfo& infos);
    virtual float interval() const;
    void redraw();

   private:
    struct Result {
      Result() : succs(-1), total(-1), prob(0.0) { }
      Result(const Result& r) : succs(r.succs), total(r.total), prob(r.prob) { }

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

    static void print(const char* label, int i, bool small, const Result& r);

    bool activated;
    char buf[256];
    int offset;
    Result last;
    Result best;

    static const int SMALL_FONT = GFUI_FONT_LARGE_C;
    static const int BIG_FONT = GFUI_FONT_BIG_C;
    static const int CENTER_ALIGN = GFUI_ALIGN_HC_VT;
  };

  virtual ~cWorldModel() {}

  virtual int priority() const;
  virtual void handle(cDriver& context);

  void addListener(cListener* listener);

 private:
  static tCarInfo build_car_info(double now, tCarElt* car);

  wrapped_container< std::vector<cListener*> > listeners;
};

#endif

