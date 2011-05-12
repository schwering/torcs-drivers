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
  return (car->_trkPos.toRight - 2.0f) / car->_trkPos.seg->width;
}

inline float mps2kmph(float mps)
{
  return mps * 3600.0f / 1000.0f;
}

inline float kmph2mps(float kmph)
{
  return kmph * 1000.0f / 3600.0f;
}

inline float metersInSeconds(const tCarElt* car, float secs)
{
  const float mps = car->_speed_x;
  return mps * secs;
}

}

#endif

