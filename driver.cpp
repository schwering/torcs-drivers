#include "driver.h"

#include <algorithm>

#include "util.h"

using std::vector;

cDriver::tfFactory cDriver::factories[MAX_HANDLERS];
int cDriver::nFactories = 0;

bool cDriver::registerHandlerFactory(tfFactory factory)
{
  if (nFactories < MAX_HANDLERS) {
    factories[nFactories++] = factory;
    return true;
  } else {
    return false;
  }
}

bool cDriver::cHandler::hasHigherPriority(const cHandler* a, const cHandler* b)
{
  return a->priority() < b->priority();
}

cDriver::cDriver()
  : track(NULL),
    car(NULL),
    sit(NULL)
{
  for (int i = 0; i < nFactories; ++i) {
    tfFactory factory = factories[i];
    cHandler* handler = factory();
    handlers.push_back(handler);
  }
  std::sort(handlers.begin(), handlers.end(), &cHandler::hasHigherPriority);
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
  for (vector<cHandler*>::const_iterator it = handlers.begin();
       it != handlers.end(); ++it) {
    cHandler* handler = *it;
    handler->handle(*this);
  }

  assertInRange(-1.0f, 1.0f, car->_steerCmd);
  assertInRange( 0.0f, 1.0f, car->_accelCmd);
  assertInRange( 0.0f, 1.0f, car->_brakeCmd);
  assertInRange( 0.0f, 1.0f, car->_clutchCmd);
  assertInRange(-1,  6,  car->_gearCmd);
}

void cDriver::endRace()
{
}

