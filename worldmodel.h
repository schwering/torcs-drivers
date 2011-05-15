#ifndef worldmodelH
#define worldmodelH

#include <map>
#include <vector>

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
  };

  class cSimplePrologSerializor : public cListener
  {
   public:
    cSimplePrologSerializor();
    virtual ~cSimplePrologSerializor() {}
    virtual void process(const tCarInfo& ci);
   private:
    bool activated;
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

