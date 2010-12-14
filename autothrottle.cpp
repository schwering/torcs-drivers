#include "autothrottle.h"

#include <tmath/v2_t.h>

#include "trackprofile.h"
#include "macros.h"

REGISTER_HANDLER(cAutoThrottle);

int cAutoThrottle::priority() const {
  return 500;
}

void cAutoThrottle::handle(cDriver& state)
{
  this->state = &state;
  tCarElt* car = state.car;

  const float speedKmh = currentSpeed();
  if (speedKmh < 50.0f) {
    return;
  }
  //LOG("speed %f %f %f %f %f\n", car->_speed_x, car->_speed_y, car->_speed_z, car->_speed_X, car->_speed_Y);
  const float curPos = currentPos();
  const int meter = (int) curPos;
  float slope;
  if ((slope = profileSlopeAverage(meter + mis(3), 12, 3)) > 90.0) {
    LOG("FULL THROTTLE %f\n", slope);
    car->_accelCmd = 0.0f;
    car->_brakeCmd = 1.0f;
  } else if ((slope = profileSlopeAverage(meter + mis(3), 20)) > 50.0) {
    LOG("MEDIUM THROTTLE %f\n", slope);
    car->_accelCmd = 0.0f;
    car->_brakeCmd = 0.7f;
  } else if ((slope = profileSlopeAverage(meter + mis(3), mis(1))) > 20.0) {
    LOG("LITTLE THROTTLE %f\n", slope);
    car->_accelCmd = 0.5f;
    car->_brakeCmd = 0.5f;
  } else if ((slope = profileSlopeAverage(meter + mis(2), mis(1))) > 9.0) {
    LOG("VERY LITTLE THROTTLE %f\n", slope);
    car->_accelCmd = 0.7f;
    car->_brakeCmd = 0.3f;
  } else if ((slope = profileSlopeAverage(meter + mis(1), mis(1))) > 2.0) {
    LOG("NO ACCEL %f\n", slope);
    car->_accelCmd = 0.0f;
    car->_brakeCmd = 0.0f;
  } else {
    //slope = profileSlopeAverage(meter, 50);
    //LOG("nothing %f\n", slope);
  }
}

float cAutoThrottle::currentPos() const
{
  const tCarElt* car = state->car;
  const tTrkLocPos pos = car->_trkPos;
  const tTrackSeg* seg = pos.seg;
  const float length = seg->lgfromstart + pos.toStart;
  return length;
}

float cAutoThrottle::mis(int secs) const
{
  const tCarElt* car = state->car;
  const float mps = car->_speed_x;
  const float metersInSecs = mps * secs;
  return metersInSecs;
}

float cAutoThrottle::currentSpeed() const
{
  const tCarElt* car = state->car;
  return car->_speed_x * 60 * 60 / 1000;
}

float cAutoThrottle::profileSlopeAverage(int fromMeter,
                                         int length,
                                         int granularity) const
{
  const cTrackProfile& profile = state->trackProfile;
  assert(length != 0);
  long double sum = 0.0f;
  cTrackProfile::cSample last;
  bool init = false;
  for (int i = 0; i < length; i += granularity) {
    const cTrackProfile::cSample& sample = profile[fromMeter + i];
    if (!init) {
      last = sample;
      init = true;
    } else {
      const cTrackProfile::cVector diff = sample - last;
      sum += diff.y / diff.x;
      last = sample;
    }
  }
  const int cnt = length / granularity - 1;
  const float avg = (float) (sum / cnt);
  const float absAvg = ((avg < 0.0f) ? -1.0f : 1.0f) * avg;
  return absAvg;
}

