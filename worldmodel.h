#ifndef worldmodelH
#define worldmodelH

#include <map>
#include <vector>

#include "driver.h"

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
    virtual void process(const tCarInfo& carInfo) = 0;
  };

  class cSimplePrologSerializor : public cListener
  {
   public:
    virtual void process(const tCarInfo& ci);
  };

  virtual int priority() const;
  virtual void handle(cDriver& state);

  void addListener(cListener* listener);

 private:
  void fireEvents(double time, const tCarElt* car);

  std::map<int, double> times;
  std::vector<cListener*>  listeners;
};

#endif

