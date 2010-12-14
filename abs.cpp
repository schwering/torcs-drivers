#include "abs.h"

#include "trackprofile.h"
#include "macros.h"

//REGISTER_HANDLER(cAntiLockBrakeSystem);

int cAntiLockBrakeSystem::priority() const {
  return 1000;
}

void cAntiLockBrakeSystem::handle(cDriver& state)
{
  tCarElt* car = state.car;
  tSituation* sit = state.sit;

  const double diff = sit->currentTime - lastTime;
  if (car->_brakeCmd > 0.0f) {
    if (diff > 0.01) {
      if (lastActive) {
        car->_brakeCmd = 0.0f;
      }
      lastActive = !lastActive;
      lastTime = sit->currentTime;
      LOG("ABS ACTIVE\n");
    }
  } else {
    lastActive = false;
  }
}

