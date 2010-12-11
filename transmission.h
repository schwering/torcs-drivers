#ifndef transmissionH
#define transmissionH

#include "driver.h"

class cTransmission : public cDriver::cHandler {
 public:
  cTransmission();

  virtual int priority() const;
  virtual void handle(cDriver& state);

 private:
  enum ShiftDirection { UP, DOWN };

  double         lastShiftTime;
  ShiftDirection lastShiftDir;
};

#endif

