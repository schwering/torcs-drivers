#ifndef driverH
#define driverH

#include <car.h>
#include <raceman.h>
#include <robot.h>
#include <track.h>

#include "macros.h"

class cDriver
{
 public:
  cDriver();
  ~cDriver();

  void initTrack(tTrack* track, tSituation* sit);
  void newRace(tCarElt* car, tSituation* sit);
  void drive(tCarElt* car, tSituation* sit);
  void endRace(tCarElt* car, tSituation* sit);

 protected:
  enum ShiftDirection { UP, DOWN };

  // TODO virtual and impls to subclass
  void initTrack();
  void newRace();
  void drive();
  void endRace();

  void adjustGear();

  tTrack*     track;
  tCarElt*    car;
  tSituation* sit;

  double         lastShiftTime;
  ShiftDirection lastShiftDir;

 private:
  DISALLOW_COPY_AND_ASSIGN(cDriver);
};

#endif

