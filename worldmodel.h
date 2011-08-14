#ifndef worldmodelH
#define worldmodelH

#include <map>
#include <vector>

#include <tgfclient.h>

#include "driver.h"
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
  };

  class cListener
  {
   public:
    virtual ~cListener() {}
    virtual void process(const tCarInfo& carInfo) = 0;
    virtual float interval() const = 0;
  };

  class cSimplePrologSerializor : public cListener
  {
   public:
    cSimplePrologSerializor();
    virtual ~cSimplePrologSerializor();
    virtual void process(const tCarInfo& ci);
    virtual float interval() const;
   private:
    bool activated;
    tCtrlMouseInfo* mouseInfo;
  };

  class cGraphicDisplay : public cListener
  {
   public:
    cGraphicDisplay();
    virtual ~cGraphicDisplay();
    virtual void process(const tCarInfo& ci);
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
  virtual void handle(cDriver& state);

  void addListener(cListener* listener);

 private:
  void fireEvents(double time, tCarElt* car);

  std::map<int, double> times;
  wrapped_container< std::vector<cListener*> > listeners;
};

#endif

