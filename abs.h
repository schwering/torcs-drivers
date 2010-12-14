#ifndef absH
#define absH

#include "driver.h"

class cAntiLockBrakeSystem : public cDriver::cHandler
{
 public:
  cAntiLockBrakeSystem() : lastTime(0.0), lastActive(false) { }

  virtual int priority() const;
  virtual void handle(cDriver& state);

 private:
  double  lastTime;
  bool    lastActive;
};

#endif

