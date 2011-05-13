#include "worldmodel.h"

#include "macros.h"
#include "util.h"

int cWorldModel::priority() const {
  return 10000;
}

void cWorldModel::handle(cDriver& state)
{
  static int j = 0;
  if (++j % 30 != 0) return;
  for (int i = 0; i < state.sit->_ncars; ++i) {
    process(state.sit->currentTime, state.sit->cars[i]);
  }
  printf("\n");
}

void cWorldModel::process(double time, const tCarElt* car)
{
  const char* name = car->_name;
  const float veloc = car->_speed_x;
  const float accel = car->_accel_x;
  const tTrkLocPos trkPos = car->_trkPos;
  float yaw = car->_yaw - RtTrackSideTgAngleL(const_cast<tTrkLocPos*>(&trkPos));
  NORM_PI_PI(yaw);
  const tTrackSeg* seg = trkPos.seg;
  const float dist = seg->lgfromstart + trkPos.toStart;
  printf("%lf: %7s v = %5.2fm/s = %5.1fkm/h f = %6.2fm/s^2 "\
         "yaw = %5.2frad = %5.1fdeg dist=%.0fm\n",
         time, name, veloc, mps2kmph(veloc), accel,
         yaw, rad2deg(yaw), dist);
}

