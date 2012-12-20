#ifndef simpledriverH
#define simpledriverH

#include "driver.h"

class cSimpleDriver : public cDriver::cHandler
{
 public:
  enum eOrientation { ORI_LEFT, ORI_MIDDLE, ORI_RIGHT };

  explicit cSimpleDriver(eOrientation ori = ORI_MIDDLE);
  virtual ~cSimpleDriver() {}

  virtual int priority() const;
  virtual void handle(cDriver& state);

 private:
  eOrientation ori;
};

#endif

