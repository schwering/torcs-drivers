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
#include "abs.h"
#include "autothrottle.h"
#include "delay.h"
#include "minithrottle.h"
#include "observer.h"
#include "simpledriver.h"
#include "transmission.h"

#include "../scenario.h"

#define MAX_BOTS 4

cDriver* drivers[MAX_BOTS] = { NULL, NULL, NULL, NULL/*, NULL, NULL, NULL, NULL, NULL, NULL*/ };

static cDriver& get_driver(int index);
static void initTrack(int         index,
                      tTrack*     track,
                      void*       carHandle,
                      void**      carParmHandle,
                      tSituation* sit);
static void newRace(int index, tCarElt* car, tSituation* s);
static void drive(int index, tCarElt* car, tSituation* s);
static void endRace(int index, tCarElt* car, tSituation* s);
static void shutdown(int index);
static int initFuncPt(int index, void* pt);

/* Configure the index-th driver and return it. */
static cDriver& get_driver(int index)
{
  if (index >= MAX_BOTS) {
    printf("access to %d/%d bot not allowed (1)\n", index, MAX_BOTS);
    exit(1);
  }
  /* The configurations should be synced with the description array in chs(). */
  if (!drivers[index]) {
    drivers[index] = new cDriver();
    drivers[index]->addHandler(new cTransmission());
    switch (index) {
      case 0: {
        cObserver* observer = new cObserver();
        //observer->addListener(new cObserver::cMercuryClient());
        observer->addListener(new cObserver::cGraphicInfoDisplay());
        observer->addListener(new cObserver::cGraphicPlanRecogDisplay());
        drivers[index]->addHandler(observer);
        drivers[index]->addHandler(new cSimpleDriver(cSimpleDriver::ORI_MIDDLE));
        drivers[index]->addHandler(new cMiniThrottle(10.0f));
        drivers[index]->addHandler(new cDelay(5.0));
        break;
      }
      case 1: {
        drivers[index]->addHandler(new cSimpleDriver(cSimpleDriver::ORI_RIGHT));
        drivers[index]->addHandler(new cMiniThrottle(59.0f));
        break;
      }
      case 2: {
        drivers[index]->addHandler(new cSimpleDriver(cSimpleDriver::ORI_RIGHT));
        drivers[index]->addHandler(new cMiniThrottle(50.0f));
        break;
      }
      case 3: {
        drivers[index]->addHandler(new cSimpleDriver(cSimpleDriver::ORI_LEFT));
        drivers[index]->addHandler(new cMiniThrottle(60.0f));
        drivers[index]->addHandler(new cDelay(5));
        break;
      }
    }
  }
  return *drivers[index];
}


/* Module entry point */
extern "C"
int chs(tModInfo* modInfo)
{
  /* The description array should be synced with the bot configurations in
   * get_driver(). These descriptions are used to build the name which is
   * displayed to the user when he configures the race. */
  const char *description[MAX_BOTS];
  for (int i = 0; i < MAX_BOTS; ++i) {
    description[i] = "";
  }
  description[0] = "Observer (right, 10km/h, 15s delay)";
  description[1] = "Passable (right, 59km/h)";
  description[2] = "Passable (right, 50km/h)";
  description[3] = "Passing (left lane, 60km/h, 5s delay)";

  memset(modInfo, 0, MAX_BOTS*sizeof(tModInfo));
  for (int i = 0; i < MAX_BOTS; ++i) {
    const char id = 'a' + i;
    char name[512];
    sprintf(name, "%c (%s)", id, description[i]);
    modInfo[i].name    = strdup(name);
    modInfo[i].desc    = strdup(name);
    modInfo[i].fctInit = initFuncPt;
    modInfo[i].gfId    = ROB_IDENT;
    modInfo[i].index   = i;
  }
  return 0;
}

/* Module interface initialization. */
static int initFuncPt(int index, void* pt)
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
  get_driver(index).initTrack(track, sit);
  *carParmHandle = NULL;
}

/* Start a new race. */
static void newRace(int index, tCarElt* car, tSituation* sit)
{
  get_driver(index).newRace(car, sit);
}

/* Drive during race. */
static void drive(int index, tCarElt* car, tSituation* sit)
{
  memset(&car->ctrl, 0, sizeof(tCarCtrl));
  car->ctrl.brakeCmd = 1.0; /* all brakes on ... */
  get_driver(index).drive(car, sit);
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
  get_driver(index).endRace(car, sit);
}

/* Called before the module is unloaded */
static void shutdown(int index)
{
  if (drivers[index]) {
    delete drivers[index];
    drivers[index] = NULL;
  }
}

