#ifndef simpledriverH
#define simpledriverH

#include "driver.h"

class cSimpleDriver : public cDriver::cHandler
{
 public:
  virtual int priority() const;
  virtual void handle(cDriver& state);
};

#endif

