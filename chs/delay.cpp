#include "delay.h"

cDelay::cDelay(float delayUntilTime)
  : delayUntilTime(delayUntilTime)
{
}

int cDelay::priority() const {
  return 1000;
}

void cDelay::handle(cDriver& state)
{
  if (state.sit->currentTime < delayUntilTime) {
    state.car->_accelCmd = 0.0f;
    state.car->_brakeCmd = 0.0f;
  }
}

