#include "worldmodel.h"

#include <sys/select.h>

#include "macros.h"
#include "util.h"

int cWorldModel::priority() const {
  return 10000;
}

void cWorldModel::handle(cDriver& state)
{
  for (int i = 0; i < state.sit->_ncars; ++i) {
    const double now = state.sit->currentTime;
    tCarElt* car = state.sit->cars[i];
    if (times.find(car->index) == times.end() ||
        now - times[car->index] > 0.5) {
      fireEvents(now, state.sit->cars[i]);
      times[car->index] = now;
    }
  }
}

void cWorldModel::fireEvents(double time, tCarElt* car)
{
  tCarInfo ci;
  ci.name = car->_name;
  ci.time = time;
  ci.veloc = car->_speed_x;
  ci.accel = car->_accel_x;
  const tTrkLocPos trkPos = car->_trkPos;
  ci.yaw = car->_yaw - RtTrackSideTgAngleL(const_cast<tTrkLocPos*>(&trkPos));
  NORM_PI_PI(ci.yaw);
  const tTrackSeg* seg = trkPos.seg;
  ci.pos = RtGetDistFromStart(car);
  ci.offset = -1.0f * trkPos.toMiddle;

  for (std::vector<cListener*>::const_iterator it = listeners.begin();
       it != listeners.end(); ++it) {
    cListener* listeners = *it;
    listeners->process(ci);
  }
}

void cWorldModel::addListener(cWorldModel::cListener* listener)
{
  listeners.push_back(listener);
}


cWorldModel::cSimplePrologSerializor::cSimplePrologSerializor()
  : activated(false)
{
}

void cWorldModel::cSimplePrologSerializor::process(
    const cWorldModel::tCarInfo& ci)
{
  if (!activated) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    int retval = select(1, &rfds, NULL, NULL, &tv);
    if (retval == -1) {
      perror("select()");
    } else if (retval) {
      activated = fgetc(stdin) != EOF;
    }
  }
  if (activated) {
    printf("(pos('%s') = %f & "\
           "offset('%s') = %f & "\
           "veloc('%s') = %f & "\
           "rad('%s') = %f & " \
           "deg('%s') = %.0f, %lf),\n",
           ci.name, ci.pos,
           ci.name, ci.offset,
           ci.name, ci.veloc,
           ci.name, ci.yaw,
           ci.name, rad2deg(ci.yaw),
           ci.time);
  }
}

