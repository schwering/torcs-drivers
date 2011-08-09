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
  //ci.yaw += M_PI / 2; // we use X axis as offset, Y as position in Golog
  NORM_PI_PI(ci.yaw);
  ci.pos = RtGetDistFromStart(car);
  ci.offset = trkPos.toMiddle;
  //ci.offset *= -1.0f; // we use X axis as offset, Y as position in Golog

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
  mouseInfo = GfctrlMouseInit();
}

cWorldModel::cSimplePrologSerializor::~cSimplePrologSerializor()
{
  GfctrlMouseRelease(mouseInfo);
}

void cWorldModel::cSimplePrologSerializor::process(
    const cWorldModel::tCarInfo& ci)
{
  if (!activated) {
    GfctrlMouseGetCurrent(mouseInfo);
    if (mouseInfo->button[0] == 1 ||
        mouseInfo->edgedn[0] == 1 ||
        mouseInfo->edgeup[0] == 1) {
      activated = true;
    }
  }

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
    if (!strcmp("human", ci.name) && abs(rad2deg(ci.yaw)) > 1.5)
      printf("observe(%2.5lf, deg('%s') = %2.2lf);\n",
             ci.time, ci.name, rad2deg(ci.yaw));
#if 0
    printf("obs(%lf, ["\
           "pos('%s') = %f, "\
           "offset('%s') = %f, "\
           "veloc('%s') = %f, "\
           "rad('%s') = %f, " \
           "deg('%s') = %f]).\n",
           ci.time,
           ci.name, ci.pos,
           ci.name, ci.offset,
           ci.name, ci.veloc,
           ci.name, ci.yaw,
           ci.name, rad2deg(ci.yaw));
#endif
  }
}

