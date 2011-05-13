#ifndef worldmodelH
#define worldmodelH

#include "driver.h"

class cWorldModel : public cDriver::cHandler
{
 public:
  struct CarInfo
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
    virtual void processCarInfo(const CarInfo& carInfo);
  };

  virtual int priority() const;
  virtual void handle(cDriver& state);

  void addListener(cListener* listener);

 private:
  void process(double time, const tCarElt* car);
};

#endif

