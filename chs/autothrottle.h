#ifndef autothrottleH
#define autothrottleH

#include "driver.h"

class cAutoThrottle : public cDriver::cHandler
{
 public:
  virtual ~cAutoThrottle() {}

  virtual int priority() const;
  virtual void handle(cDriver& state);

 private:
  float mis(const cDriver& state, float secs) const;
  float profileSlopeAverage(const cDriver& state,
                            int fromMeter,
                            int length,
                            int granularity = 10) const;
};

#endif

