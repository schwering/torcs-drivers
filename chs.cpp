#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <tgf.h>
#include <track.h>
#include <car.h>
#include <raceman.h>
#include <robottools.h>
#include <robot.h>

#include "driver.h"

#define MAX_BOTS 10

cDriver drivers[MAX_BOTS];
int driverCount = 0;

static void initTrack(int         index,
                      tTrack*     track,
                      void*       carHandle,
                      void**      carParmHandle,
                      tSituation* sit);
static void newRace(int index, tCarElt* car, tSituation* s);
static void drive(int index, tCarElt* car, tSituation* s);
static void endRace(int index, tCarElt* car, tSituation* s);
static void shutdown(int index);
static int  initFuncPt(int index, void* pt);

/* Module entry point */
extern "C"
int chs(tModInfo* modInfo)
{
  memset(modInfo, 0, 10*sizeof(tModInfo));
  modInfo->name  = const_cast<char*>("chs");
  modInfo->desc  = const_cast<char*>("");
  modInfo->fctInit = initFuncPt;
  modInfo->gfId  = ROB_IDENT;
  modInfo->index   = 1;
  return 0;
}

/* Module interface initialization. */
static int
initFuncPt(int index, void* pt)
{
  tRobotItf* itf = (tRobotItf*) pt;

  itf->rbNewTrack = initTrack;  /* Give the robot the track view called */
                  /* for every track change or new race */
  itf->rbNewRace  = newRace;  /* Start a new race */
  itf->rbDrive  = drive;  /* Drive during race */
  itf->rbPitCmd   = NULL;
  itf->rbEndRace  = endRace;  /* End of the current race */
  itf->rbShutdown = shutdown;   /* Called before the module is unloaded */
  itf->index    = index;  /* Index used if multiple interfaces */
  return 0;
}

/* Called for every track change or new race. */
static void initTrack(int index,
                      tTrack*     track,
                      void*       carHandle,
                      void**      carParmHandle,
                      tSituation* sit)
{
  drivers[index].initTrack(track, sit);
  *carParmHandle = NULL;
}

/* Start a new race. */
static void newRace(int index, tCarElt* car, tSituation* sit)
{
  drivers[index].newRace(car, sit);
}

/* Drive during race. */
static void drive(int index, tCarElt* car, tSituation* sit)
{
  memset(&car->ctrl, 0, sizeof(tCarCtrl));
  car->ctrl.brakeCmd = 1.0; /* all brakes on ... */
  drivers[index].drive(car, sit);
  /*
   * add the driving code here to modify the
   * car->_steerCmd
   * car->_accelCmd
   * car->_brakeCmd
   * car->_gearCmd
   * car->_clutchCmd
   */
}

/* End of the current race */
static void endRace(int index, tCarElt* car, tSituation* sit)
{
  drivers[index].endRace(car, sit);
}

/* Called before the module is unloaded */
static void shutdown(int index)
{
  //drivers[index].destroy();
}

