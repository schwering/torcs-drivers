#ifndef delayH
#define delayH

#include "driver.h"

class cDelay : public cDriver::cHandler
{
 public:
  explicit cDelay(float delayUntilTime);
  virtual ~cDelay() {}

  virtual int priority() const;
  virtual void handle(cDriver& state);

 private:
  float delayUntilTime;
};

#endif

