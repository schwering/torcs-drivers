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
  return mps * 3.6f;
}

inline float kmph2mps(float kmph)
{
  return kmph / 3.6f;
}

inline float metersInSeconds(const tCarElt* car, float secs)
{
  const float mps = car->_speed_x;
  return mps * secs;
}

inline float rad2deg(float rad)
{
  const double pi = 3.1415926535897931;
  return (float)((double) rad / pi * 180.0);
}

inline float deg2rad(float deg)
{
  const double pi = 3.1415926535897931;
  return (float)((double) deg / 180.0 * pi);
}

}

#endif

