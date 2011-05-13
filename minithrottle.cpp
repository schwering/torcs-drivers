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
  car->_accelCmd = restrictRange(0.0f, 1.0f, diff / 10.0);
  car->_brakeCmd = restrictRange(0.0f, 1.0f, -1.0f * diff / 10.0);
}

