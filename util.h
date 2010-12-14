#ifndef utilH
#define utilH

#include <assert.h>

#include <robottools.h>

namespace {

inline float angleRelToTrack(tCarElt* car)
{
  assert(car);
  float angle = RtTrackSideTgAngleL(&(car->_trkPos)) - car->_yaw;
  NORM_PI_PI(angle);
  return angle;
}

inline float relativeYPos(tCarElt* car)
{
  assert(car);
  assert(car->_trkPos.seg->width != 0.0f);
  return car->_trkPos.toMiddle / car->_trkPos.seg->width;
}

}

#endif

