#ifndef minithrottleH
#define minithrottleH

#include "driver.h"

class cMiniThrottle : public cDriver::cHandler
{
 public:
  virtual int priority() const;
  virtual void handle(cDriver& state);
};

#endif

