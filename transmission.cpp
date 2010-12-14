#include "transmission.h"

#include "util.h"

REGISTER_HANDLER(cTransmission);

cTransmission::cTransmission()
  : lastShiftTime(0.0),
    lastShiftDir(DOWN)
{
}

int cTransmission::priority() const {
  return 100;
}

void cTransmission::handle(cDriver& state)
{
  tCarElt* car = state.car;
  tSituation* sit = state.sit;

  const float max = car->_enginerpmRedLine;
  const double now = sit->currentTime;
  if (inRange(0.0, 3.0, now)) {
    car->_gearCmd = 1;
    lastShiftTime = now;
    lastShiftDir = UP;
  } else if (car->_enginerpm > 0.95 * max &&
             car->_gear < 6 &&
             (lastShiftTime + 2.0 < now || lastShiftDir == UP)) {
    car->_gearCmd = car->_gear + 1;
    lastShiftTime = now;
    lastShiftDir = UP;
  } else if (car->_gear > 1 &&
             car->_enginerpm < 0.1 * max &&
             car->_gear > 1 &&
             (lastShiftTime + 2.0 < now || lastShiftDir == DOWN)) {
    car->_gearCmd = car->_gear - 1;
    lastShiftTime = now;
    lastShiftDir = DOWN;
  } else {
    car->_gearCmd = car->_gear;
  }
}

