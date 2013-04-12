#include "simpledriver.h"

#include "util.h"

cSimpleDriver::cConstantLane::cConstantLane(eOrientation ori, float slope)
  : ori(ori),
    slope_(slope)
{
}

cSimpleDriver::eOrientation
cSimpleDriver::cConstantLane::lane(const cDriver& context)
{
  return ori;
}

float cSimpleDriver::cConstantLane::slope(const cDriver& context) const
{
  return slope_;
}

cSimpleDriver::cLaneTeller* cSimpleDriver::cConstantLane::clone() const
{
  return new cConstantLane(*this);
}

cSimpleDriver::cOvertakeWhenNeeded::cOvertakeWhenNeeded(eOrientation ori, float slope)
  : ori(ori),
    slope_(slope)
{
}

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

cSimpleDriver::eOrientation
cSimpleDriver::cOvertakeWhenNeeded::lane(const cDriver& context)
{
  for (int i = 0; i < context.sit->_ncars; ++i) {
    if (context.sit->cars[i] != context.car) {
      tCarElt* me = context.car;
      tCarElt* other = context.sit->cars[i];
      if (me->_speed_x > other->_speed_x &&
          RtGetDistFromStart(me) < RtGetDistFromStart(other) &&
          RtGetDistFromStart(me) > RtGetDistFromStart(other) - 35.0f &&
          sgn(me->_trkPos.toMiddle) == sgn(other->_trkPos.toMiddle)) {
        switch (ori) {
          case ORI_LEFT:  ori = ORI_RIGHT; break;
          case ORI_RIGHT: ori = ORI_LEFT;  break;
          default:        assert(false);
        }
      }
    }
  }
  return ori;
}

float cSimpleDriver::cOvertakeWhenNeeded::slope(const cDriver& context) const
{
  return slope_;
}

cSimpleDriver::cLaneTeller* cSimpleDriver::cOvertakeWhenNeeded::clone() const
{
  return new cOvertakeWhenNeeded(*this);
}

cSimpleDriver::cRandomLaneChanges::cRandomLaneChanges(eOrientation ori, float slope)
  : ori(ori),
    slope_(slope)
{
}

cSimpleDriver::eOrientation
cSimpleDriver::cRandomLaneChanges::lane(const cDriver& context)
{
  if (rand() % (1024 * 1024) == 0) {
    switch (ori) {
      case ORI_LEFT:  ori = ORI_RIGHT; break;
      case ORI_RIGHT: ori = ORI_LEFT;  break;
      default:        assert(false);
    }
  }
  return ori;
}

float cSimpleDriver::cRandomLaneChanges::slope(const cDriver& context) const
{
  return slope_;
}

cSimpleDriver::cLaneTeller* cSimpleDriver::cRandomLaneChanges::clone() const
{
  return new cRandomLaneChanges(*this);
}

cSimpleDriver::cSwitchAfter::cSwitchAfter(double time,
                                          const cSimpleDriver::cLaneTeller& t1,
                                          const cSimpleDriver::cLaneTeller& t2)
  : time(time),
    t1(t1.clone()),
    t2(t2.clone())
{
}

cSimpleDriver::cSwitchAfter::~cSwitchAfter()
{
  delete t1;
  delete t2;
}

cSimpleDriver::eOrientation
cSimpleDriver::cSwitchAfter::lane(const cDriver& context)
{
  if (context.sit->currentTime < time) {
    return t1->lane(context);
  } else {
    return t2->lane(context);
  }
}

float cSimpleDriver::cSwitchAfter::slope(const cDriver& context) const
{
  if (context.sit->currentTime < time) {
    return t1->slope(context);
  } else {
    return t2->slope(context);
  }
}

cSimpleDriver::cLaneTeller*
cSimpleDriver::cSwitchAfter::clone() const
{
  return new cSwitchAfter(time, *t1, *t2);
}

cSimpleDriver::cSimpleDriver(const cLaneTeller& laneTeller)
  : laneTeller(laneTeller.clone())
{
}

cSimpleDriver::~cSimpleDriver()
{
  delete laneTeller;
}

int cSimpleDriver::priority() const {
  return 200;
}

void cSimpleDriver::handle(cDriver& context)
{
  if (context.sit->currentTime < 0.0) {
    return;
  }

  tCarElt* car = context.car;

  const float roadAngle = angleRelToTrack(car);
  const tTrkLocPos& pos = car->_trkPos;
  const tTrackSeg& seg = *(pos.seg);
  float desiredRelativeOffsetChange;
  switch (laneTeller->lane(context)) {
    case ORI_LEFT:
      desiredRelativeOffsetChange = -1.0f * (pos.toLeft - 2.0f) / seg.width;
      break;
    case ORI_MIDDLE:
      desiredRelativeOffsetChange = pos.toMiddle / seg.width;
      break;
    case ORI_RIGHT:
      desiredRelativeOffsetChange = (pos.toRight - 2.0f) / seg.width;
      break;
    default:
      assert(false);
  }
  const float midAngle = -1.0f * desiredRelativeOffsetChange;

  const float steerAngle = roadAngle + midAngle;
  const float steer = steerAngle / car->_steerLock * laneTeller->slope(context);
  car->_steerCmd = restrictRange(-1.0f, 1.0f, steer);
  car->_accelCmd = 1.0f;
  car->_brakeCmd = 0.0f;
}

