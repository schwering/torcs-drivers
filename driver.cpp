#include "driver.h"

#include <algorithm>

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
  trackProfile.init(track);
  FILE* fp = fopen("/tmp/track_points", "w");
  assert(fp);
  cTrackProfile::cVector prev(0.0f, 0.0f);
  for (cTrackProfile::const_iterator it = trackProfile.begin();
       it != trackProfile.end(); ++it) {
    const cTrackProfile::cSample sample = *it;
    const cTrackProfile::cVector diff = sample - prev;
    prev = sample;
    fprintf(fp, "%f %f %f\n", sample.x, sample.y, diff.y/diff.x);
  }
  fclose(fp);
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

  assertInRangeWarn(-1.0f, 1.0f, car->_steerCmd);
  assertInRangeWarn( 0.0f, 1.0f, car->_accelCmd);
  assertInRangeWarn( 0.0f, 1.0f, car->_brakeCmd);
  assertInRangeWarn( 0.0f, 1.0f, car->_clutchCmd);
  assertInRangeWarn(-1,  6,  car->_gearCmd);
}

void cDriver::endRace()
{
}

