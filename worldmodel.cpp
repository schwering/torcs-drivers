#include "worldmodel.h"

#include "macros.h"
#include "util.h"

int cWorldModel::priority() const {
  return 10000;
}

void cWorldModel::handle(cDriver& state)
{
  for (int i = 0; i < state.sit->_ncars; ++i) {
    const double now = state.sit->currentTime;
    const tCarElt* car = state.sit->cars[i];
    if (times.find(car->index) == times.end() ||
        now - times[car->index] > 0.5) {
      fireEvents(now, state.sit->cars[i]);
      times[car->index] = now;
    }
  }
}

void cWorldModel::fireEvents(double time, const tCarElt* car)
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
  ci.pos = seg->lgfromstart + trkPos.toStart;
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

void cWorldModel::cSimplePrologSerializor::process(
    const cWorldModel::tCarInfo& ci)
{
  printf("(pos('%s') = %f & "\
         "offset('%s') = %f & "\
         "veloc('%s') = %f & "\
         "yaw('%s') = %f, %lf),\n",
         ci.name, ci.pos,
         ci.name, ci.offset,
         ci.name, ci.veloc,
         ci.name, ci.yaw,
         ci.time);
}

