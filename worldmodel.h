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

  class cGraphicDisplay : public cListener
  {
   public:
    cGraphicDisplay();
    virtual ~cGraphicDisplay();
    virtual void process(const cDriver& context,
                         const tCarInfo& info);
    virtual float interval() const;
   private:
    static const float WHITE[4];
    static const int FONT = GFUI_FONT_SMALL_C;
    static const int RIGHT_ALIGN = GFUI_ALIGN_HL_VT;
    static const int LEFT_ALIGN = GFUI_ALIGN_HL_VC;
    static const int X = 80;
    static const int COLUMN_WIDTH = 50;
    static const int Y[5];
    int col;
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

