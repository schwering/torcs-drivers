#ifndef minithrottleH
#define minithrottleH

#include "driver.h"

class cMiniThrottle : public cDriver::cHandler
{
 public:
  explicit cMiniThrottle(float maxKmph = 50.0);

  virtual int priority() const;
  virtual void handle(cDriver& state);

 private:
  float maxKmph;
};

#endif

