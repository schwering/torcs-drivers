#ifndef trackprofileH
#define trackprofileH

#include <list>
#include <vector>

#include <track.h>
#include <tmath/v2_t.h>

#include "macros.h"

class cTrackProfile {
 public:
  typedef v2t<float> cVector;

  class cSample : public cVector {
   public:
    cSample() : space(0.0f) { }
    cSample(const cVector& v, float space)
        : cVector(v), space(space) { }
    cSample(float x, float y, float space)
        : cVector(x, y), space(space) { }

    float space;
  };

  typedef std::vector<cSample>::const_iterator const_iterator;

  cTrackProfile();
  virtual ~cTrackProfile();
  void init(tTrack* track);

  const_iterator begin() const { return samples.begin(); }
  const_iterator end() const { return samples.end(); }
  const cSample& operator[](size_t i) const {
    return samples[i % samples.size()];
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(cTrackProfile);

  cSample& get(int& i);
  const cSample& get(int& i) const;
  void storeSegment(const tTrackSeg* seg);

  /**
   * Dispatches to sampleCurve() or sampleStraight().
   */
  static std::list<cSample> sample(const tTrackSeg* seg);

  /**
   * Samples a single curve segment in a isolated manner, which is taking some
   * positions in the middle of the segment in 1-meter-steps and calculating
   * vectors to this point.
   * The samples are set into bigger picture in another place (this is done by
   * storeSegment() by calling clipSegments()).
   *
   * We interpret the segment as (part of a half) circle with the center
   * (x_m, y_m) = (r, 0) where r is the radius.
   * The segment starts at point (0, 0). The segment is a part of the circular
   * line in either the first (above the X-axis, i.e. positive Y) or the fourth
   * (below the X-axis, i.e. negative Y) quadrant: if the segment is a left
   * curve, it is supposed to lie in the fourth quadrant; if it is a right turn,
   * it lies in the first quadrant.
   * The segment ends after traversing l units of the circular line where l is
   * the length of the segment. The X-coordinate of l can be determined by
   * imagining a second circle around (0, 0) with radius l and calculating the
   * two circles' common point(s).
   * In general it is y = y_m +/- sqrt(r^2 - (x - x_m)^2). In our case, this
   * reduces to y_1 = +/- sqrt(r^2 - (x - r)^2) and y_2 = +/- sqrt(l^2 - x^2).
   * Their common points lie have the X-coordinate x = l^2 / 2r.
   * The Y-coordinate of a vector that points to a point of the circular line at
   * a given X-coordinate is y = y_m +/- sqrt(r^2 - (x - x_m)) which in our case
   * reduces to (-)sqrt(r^2 - (x - r)^2) or, completely without x,
   * (-)sqrt(r^2 - (2l/pi - r)^2).
   * The square root is negative if we have a left turn, because in this case we
   * want to traverse the circle counter-clockwise; otherwise it's positive.
   *
   * We don't want to just have one sample per segment, because a segment is
   * often about five meters long. So we create such vector not just for 
   *
   * Additionally, a sample has a space attached. The space means how much room
   * is at the left and right on the track of the sampled position.
   * Each segment has a starting and a ending width and we assume it increases
   * or decreases linearly and determine it in a respective way.
   */
  static std::list<cSample> sampleCurve(const tTrackSeg* seg);

  /**
   * Samples a single straight segment.
   * The idea is completely analoguous to sampleCurve(), but the calculation of
   * the Y-coordinate of the vector is trivial (0.0).
   */
  static std::list<cSample> sampleStraight(const tTrackSeg* seg);

  std::vector<cSample> samples;
};

#endif

