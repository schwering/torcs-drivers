#ifndef utilH
#define utilH

#include <assert.h>

#include <robottools.h>

#define inRange(Lo, Hi, Val)          (Lo <= Val && Val <= Hi)
#define assertInRange(Lo, Hi, Val)    assert(Lo <= Val),assert(Val <= Hi)
#define restrictRange(Lo, Hi, Val)    ((Val) < (Lo) ? (Lo) :\
                                        ((Val) > (Hi) ? (Hi) : (Val)))

static inline float angleRelToTrack(tCarElt* car)
{
    assert(car);
    float angle = RtTrackSideTgAngleL(&(car->_trkPos)) - car->_yaw;
    NORM_PI_PI(angle);
    return angle;
}

static inline float relativeYPos(tCarElt* car)
{
    assert(car);
    assert(car->_trkPos.seg->width != 0.0f);
    return car->_trkPos.toMiddle / car->_trkPos.seg->width;
}

#endif

