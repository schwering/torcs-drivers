#include "simpledriver.h"

#include "util.h"

cSimpleDriver::cSimpleDriver(eOrientation ori)
  : ori(ori)
{
}

int cSimpleDriver::priority() const {
  return 200;
}

void cSimpleDriver::handle(cDriver& state)
{
  if (state.sit->currentTime < 0.0) {
    return;
  }

  tCarElt* car = state.car;

  const float roadAngle = angleRelToTrack(car);
  const tTrkLocPos& pos = car->_trkPos;
  const tTrackSeg& seg = *(pos.seg);
  float desiredRelativeOffsetChange;
  switch (ori) {
    case ORI_LEFT:
      desiredRelativeOffsetChange = -1.0f * (pos.toLeft - 0.5f) / seg.width;
      break;
    case ORI_MIDDLE:
      desiredRelativeOffsetChange = pos.toMiddle / seg.width;
      break;
    case ORI_RIGHT:
      desiredRelativeOffsetChange = (pos.toRight - 1.0f) / seg.width;
      break;
  }
  const float midAngle = -1.0f * desiredRelativeOffsetChange;

  const float steerAngle = roadAngle + midAngle;
  const float steer = steerAngle / car->_steerLock;
  car->_steerCmd = restrictRange(-1.0f, 1.0f, steer);
  car->_accelCmd = 1.0f;
  car->_brakeCmd = 0.0f;
}

