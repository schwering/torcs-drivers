#include "driver.h"

#include "util.h"

cDriver::cDriver()
  : track(NULL),
    car(NULL),
    sit(NULL),
    lastShiftTime(0.0),
    lastShiftDir(DOWN)
{
}

cDriver::~cDriver()
{
}

void cDriver::initTrack(tTrack* track, tSituation* sit)
{
  this->track = track;
  this->sit = sit;
  initTrack();
}

void cDriver::newRace(tCarElt* car, tSituation* sit)
{
  this->car = car;
  this->sit = sit;
  newRace();
}

void cDriver::drive(tCarElt* car, tSituation* sit)
{
  this->car = car;
  this->sit = sit;
  drive();
}

void cDriver::endRace(tCarElt* car, tSituation* sit)
{
  this->car = car;
  this->sit = sit;
  endRace();
}

void cDriver::initTrack()
{
}

void cDriver::newRace()
{
}

void cDriver::drive()
{
  const float roadAngle = angleRelToTrack(car);
  const float yPos = relativeYPos(car);
  const float midAngle = -1.0f * yPos;

  const float steerAngle = roadAngle + midAngle;
  const float steer = steerAngle / car->_steerLock;
  car->_steerCmd = restrictRange(-1.0f, 1.0f, steer);
  car->_accelCmd = 1.0f;
  car->_brakeCmd = 0.0f;

  adjustGear();

  assertInRange(-1.0f, 1.0f, car->_steerCmd);
  assertInRange( 0.0f, 1.0f, car->_accelCmd);
  assertInRange( 0.0f, 1.0f, car->_brakeCmd);
  assertInRange( 0.0f, 1.0f, car->_clutchCmd);
  assertInRange(-1,  6,  car->_gearCmd);
}

void cDriver::adjustGear()
{
  const float max = car->_enginerpmRedLine;
  const double now = sit->currentTime;
  if (now <= 3.0) {
    car->_gearCmd = 1;
    lastShiftTime = now;
    lastShiftDir = UP;
  } else if (car->_enginerpm > 0.95 * max &&
             car->_gear < 6 &&
             (lastShiftTime + 2.0 < now || lastShiftDir == UP)) {
    car->_gearCmd = car->_gear + 1;
    lastShiftTime = now;
    lastShiftDir = UP;
  } else if (car->_gear > 1 &&
             car->_enginerpm < 0.1 * max &&
             car->_gear > 1 &&
             (lastShiftTime + 2.0 < now || lastShiftDir == DOWN)) {
    car->_gearCmd = car->_gear - 1;
    lastShiftTime = now;
    lastShiftDir = DOWN;
  } else {
    car->_gearCmd = car->_gear;
  }
}

void cDriver::endRace()
{
}

