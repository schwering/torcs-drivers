#include "minithrottle.h"

#include <tmath/v2_t.h>

#include "trackprofile.h"
#include "macros.h"
#include "util.h"

int cMiniThrottle::priority() const {
  return 500;
}

void cMiniThrottle::handle(cDriver& state)
{
  tCarElt* car = state.car;

  const float speedKmh = mps2kmph(car->_speed_x);
  const float diff = 50.0f - speedKmh;
  car->_accelCmd = restrictRange(0.0f, 1.0f, diff / 10.0);
  car->_brakeCmd = restrictRange(0.0f, 1.0f, -1.0f * diff / 10.0);
}

