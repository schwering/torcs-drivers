#ifndef absH
#define absH

#include "driver.h"

class cAntiLockBrakeSystem : public cDriver::cHandler
{
 public:
  virtual ~cAntiLockBrakeSystem() {}

  virtual int priority() const;
  virtual void handle(cDriver& state);

 private:
  static const float MIN_SPEED;
  static const float SLIP;
  static const float RANGE;
};

#endif

