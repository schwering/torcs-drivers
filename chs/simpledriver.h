#ifndef simpledriverH
#define simpledriverH

#include "driver.h"

class cSimpleDriver : public cDriver::cHandler
{
 public:
  enum eOrientation { ORI_LEFT, ORI_MIDDLE, ORI_RIGHT };

  class cLaneTeller {
   public:
    virtual ~cLaneTeller() { }
    virtual eOrientation lane(const cDriver& context) = 0;
    virtual float slope(const cDriver& context) const = 0;
    virtual cLaneTeller* clone() const = 0;
  };

  class cConstantLane : public cLaneTeller {
   public:
    explicit cConstantLane(eOrientation ori, float slope_ = 1.0f);
    virtual eOrientation lane(const cDriver& context);
    virtual float slope(const cDriver& context) const;
    virtual cLaneTeller* clone() const;
   private:
    eOrientation ori;
    float slope_;
  };

  class cOvertakeWhenNeeded : public cLaneTeller {
   public:
    explicit cOvertakeWhenNeeded(eOrientation ori, float slope_ = 1.0f);
    virtual eOrientation lane(const cDriver& context);
    virtual float slope(const cDriver& context) const;
    virtual cLaneTeller* clone() const;
   private:
    eOrientation ori;
    float slope_;
  };

  class cRandomLaneChanges : public cLaneTeller {
   public:
    explicit cRandomLaneChanges(eOrientation ori, float slope_ = 1.0f);
    virtual eOrientation lane(const cDriver& context);
    virtual float slope(const cDriver& context) const;
    virtual cLaneTeller* clone() const;
   private:
    eOrientation ori;
    float slope_;
  };

  class cSwitchAfter : public cLaneTeller {
   public:
    cSwitchAfter(double time, const cLaneTeller& t1, const cLaneTeller& t2);
    virtual ~cSwitchAfter();
    virtual eOrientation lane(const cDriver& context);
    virtual float slope(const cDriver& context) const;
    virtual cLaneTeller* clone() const;
   private:
    const double time;
    cLaneTeller* t1;
    cLaneTeller* t2;
  };

  explicit cSimpleDriver(const cLaneTeller& laneTeller);
  virtual ~cSimpleDriver();

  virtual int priority() const;
  virtual void handle(cDriver& context);

 private:
  cLaneTeller* laneTeller;
};

#endif

