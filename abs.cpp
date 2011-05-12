#include "abs.h"

#include "macros.h"
#include "util.h"

const float cAntiLockBrakeSystem::MIN_SPEED = kmph2mps(30.0f);
const float cAntiLockBrakeSystem::SLIP = kmph2mps(10.0f);
const float cAntiLockBrakeSystem::RANGE = kmph2mps(20.0f);

int cAntiLockBrakeSystem::priority() const {
  return 1000;
}

void cAntiLockBrakeSystem::handle(cDriver& state)
{
  tCarElt* car = state.car;

  const float carSpeed = car->_speed_x;
  if (carSpeed < MIN_SPEED) {
    return;
  }

  float wheelSpeed = 0.0f;
  for (int i = 0; i < 4; ++i) {
    wheelSpeed += car->_wheelSpinVel(i) * car->_wheelRadius(i);
  }
  wheelSpeed /= 4.0f;

  if (wheelSpeed > carSpeed + SLIP || // XXX this gives us traction control :)
      wheelSpeed < carSpeed - SLIP) {
    car->_brakeCmd -= (carSpeed - wheelSpeed - SLIP) / RANGE;
    car->_brakeCmd = restrictRange(0.0, 1.0, car->_brakeCmd);
    LOG("ABS active (brake %f)\n", car->_brakeCmd);
  }
}

