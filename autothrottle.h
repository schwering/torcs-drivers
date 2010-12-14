#ifndef autothrottleH
#define autothrottleH

#include "driver.h"

class cAutoThrottle : public cDriver::cHandler
{
 public:
  virtual int priority() const;
  virtual void handle(cDriver& state);

 private:
  float currentPos() const;
  float mis(int secs) const; // meters in seconds
  float currentSpeed() const;
  float profileSlopeAverage(int fromMeter,
                            int length,
                            int granularity = 10) const;

  const cDriver* state;
};

#endif

