#include "minithrottle.h"

#include <tmath/v2_t.h>

#include "trackprofile.h"
#include "macros.h"
#include "util.h"

cMiniThrottle::cMiniThrottle(float maxKmph)
  : maxKmph(maxKmph)
{
}

int cMiniThrottle::priority() const {
  return 500;
}

void cMiniThrottle::handle(cDriver& state)
{
  tCarElt* car = state.car;

  const float speedKmph = mps2kmph(car->_speed_x);
  const float diff = maxKmph - speedKmph;
  // I found the following constant values by trial and error (for the
  // blue cobra car), but there is some intuition behind them:
  const double BROADENING = 10.0; // stretch diff^3 function somewhat
  const double ACCELBOOST = 13.5; // accel has much less effect than brake
  const double NOACCEL = 0.15; // before braking, we can just avoid accel
  const double cmd = diff*diff*diff * BROADENING;
  car->_accelCmd = (float) restrictRange(0.0, 1.0, ACCELBOOST * cmd);
  car->_brakeCmd = -1.0f * (float) restrictRange(-1.0, 0.0, cmd + NOACCEL);
}

