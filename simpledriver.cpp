#include "simpledriver.h"

#include "util.h"

REGISTER_HANDLER(cSimpleDriver);

int cSimpleDriver::priority() const {
  return 200;
}

void cSimpleDriver::handle(cDriver& state)
{
  tCarElt* car = state.car;
  tSituation* sit = state.sit;

  const float roadAngle = angleRelToTrack(car);
  const float yPos = relativeYPos(car);
  const float midAngle = -1.0f * yPos;

  const float steerAngle = roadAngle + midAngle;
  const float steer = steerAngle / car->_steerLock;
  car->_steerCmd = restrictRange(-1.0f, 1.0f, steer);
  car->_accelCmd = 0.3f;
  car->_brakeCmd = 0.0f;
  car->_gearCmd = 1;
}

