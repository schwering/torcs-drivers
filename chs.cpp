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
#include "minithrottle.h"
#include "simpledriver.h"
#include "transmission.h"

#define MAX_BOTS 10

cDriver* drivers[MAX_BOTS] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

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
  if (!drivers[index]) {
    drivers[index] = new cDriver();
    drivers[index]->addHandler(new cTransmission());
    drivers[index]->addHandler(new cMiniThrottle());
    switch (index) {
      case 0:
        drivers[index]->addHandler(new cSimpleDriver(cSimpleDriver::ORI_RIGHT));
        printf("NULL\n");
        break;
      case 1:
        drivers[index]->addHandler(new cSimpleDriver(cSimpleDriver::ORI_LEFT));
        break;
      case 2:
        drivers[index]->addHandler(new cSimpleDriver(cSimpleDriver::ORI_MIDDLE));
        break;
      case 3:
        drivers[index]->addHandler(new cSimpleDriver(cSimpleDriver::ORI_RIGHT));
        break;
    }
  }
  return *drivers[index];
}


static const char* botname[MAX_BOTS] = {
  "chs 1", "chs 2", "chs 3", "chs 4", "chs 5",
  "chs 6", "chs 7", "chs 8", "chs 9", "chs 10"
};

static const char* botdesc[MAX_BOTS] = {
  "chs 1", "chs 2", "chs 3", "chs 4", "chs 5",
  "chs 6", "chs 7", "chs 8", "chs 9", "chs 10"
};

/* Module entry point */
extern "C"
int chs(tModInfo* modInfo)
{
  memset(modInfo, 0, MAX_BOTS*sizeof(tModInfo));
  for (int i = 0; i < MAX_BOTS; ++i) {
    modInfo[i].name    = strdup(botname[i]);
    modInfo[i].desc    = strdup(botdesc[i]);
    modInfo[i].fctInit = initFuncPt;
    modInfo[i].gfId    = ROB_IDENT;
    modInfo[i].index   = i;
  }
  for (int i = 0; i < MAX_BOTS; ++i) {
    printf("bot '%s' / '%s' / %d\n", modInfo[i].name, modInfo[i].desc, modInfo[i].index);
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

