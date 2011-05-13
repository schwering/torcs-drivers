#ifndef worldmodelH
#define worldmodelH

#include "driver.h"

class cWorldModel : public cDriver::cHandler
{
 public:
  virtual int priority() const;
  virtual void handle(cDriver& state);

 private:
  void process(double time, const tCarElt* car);
};

#endif

