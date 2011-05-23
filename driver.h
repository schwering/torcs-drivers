#ifndef driverH
#define driverH

#include <vector>

#include <car.h>
#include <raceman.h>
#include <robot.h>
#include <track.h>

#include "trackprofile.h"
#include "macros.h"
#include "wrapped_container.h"

#define MAX_HANDLERS 10
#define REGISTER_HANDLER(HandlerClass) \
    namespace {\
      cDriver::cHandler* factory() {\
        return new HandlerClass();\
      }\
      bool registered = cDriver::registerHandlerFactory(factory);\
    }

class cDriver
{
 public:
  class cHandler
  {
   public:
    inline cHandler() { }
    virtual ~cHandler() { }
    /** Handlers with highest prio. are executed last. Yes, last. */
    virtual int priority() const = 0;
    virtual void handle(cDriver& state) = 0;
    static bool hasHigherPriority(const cHandler* a, const cHandler* b);
   private:
    DISALLOW_COPY_AND_ASSIGN(cHandler);
  };
  typedef cHandler* (*tfFactory)();

  static bool registerHandlerFactory(tfFactory factory);

  cDriver();
  virtual ~cDriver() {}

  void addHandler(cHandler* handler);

  void initTrack(tTrack* track, tSituation* sit);
  void newRace(tCarElt* car, tSituation* sit);
  void drive(tCarElt* car, tSituation* sit);
  void endRace(tCarElt* car, tSituation* sit);

  tTrack*       track;
  tCarElt*      car;
  tSituation*   sit;
  cTrackProfile trackProfile;

 private:
  DISALLOW_COPY_AND_ASSIGN(cDriver);

  static tfFactory factories[];
  static int nFactories;

  void initTrack();
  void newRace();
  void drive();
  void endRace();

  wrapped_container< std::vector<cHandler*> >  handlers;
};

#endif

