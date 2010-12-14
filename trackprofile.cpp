#include "trackprofile.h"

#include <cstdlib>
#include <cmath>

#include "util.h"

cTrackProfile::cTrackProfile()
  : samples(NULL)
{
}

cTrackProfile::~cTrackProfile()
{
}

void cTrackProfile::init(tTrack* track)
{
  tTrackSeg* seg;

  seg = track->seg;
  do {
    storeSegment(seg);
    seg = seg->next;
  } while (seg != track->seg);
}

cTrackProfile::cSample& cTrackProfile::get(int& i)
{
  assertInRange(0, (int) samples.size(), i);
  i %= samples.size();
  return samples[i];
}

const cTrackProfile::cSample& cTrackProfile::get(int& i) const
{
  assertInRange(0, (int) samples.size(), i);
  i %= samples.size();
  return samples[i];
}

void cTrackProfile::storeSegment(const tTrackSeg* seg)
{
  using std::list;
  using std::vector;
  const cVector currentPos =
      (samples.empty()) ? cVector(0.0f, 0.0f) : *samples.rbegin();
  list<cSample> ss = sample(seg);
  for (list<cSample>::iterator it = ss.begin(); it != ss.end(); ++it) {
    cSample& sample = *it;
    sample += currentPos;
  }
  samples.insert(samples.end(), ss.begin(), ss.end());
}

std::list<cTrackProfile::cSample> cTrackProfile::sample(const tTrackSeg* seg)
{
  switch (seg->type) {
    case TR_STR:
      return sampleStraight(seg);
    case TR_LFT:
    case TR_RGT:
      return sampleCurve(seg);
    default:
      assert(false);
  }
}

std::list<cTrackProfile::cSample> cTrackProfile::sampleStraight(
    const tTrackSeg* seg)
{
  using std::list;
  assert(seg->type == TR_STR);
  list<cSample> samples;

  const float sw = seg->startWidth;
  const float ew = seg->endWidth;

  for (float l = 0.0f; l <= seg->length; ++l)
  {
    const float x = l;
    const float y = 0.0f;

    const float w = sw + (ew - sw) * l / seg->length;
    const float s = w / 2.0f;

    samples.push_back(cSample(x, y, s));
  }
  return samples;
}

std::list<cTrackProfile::cSample> cTrackProfile::sampleCurve(
    const tTrackSeg* seg)
{
  using std::list;
  assert(seg->type == TR_LFT || seg->type == TR_RGT);
  list<cSample> samples;
  // See method's documentation in the header for the equations.

  const float r = seg->radius;

  const float sw = seg->startWidth;
  const float ew = seg->endWidth;

  for (float l = 0.0f; l <= seg->length; ++l)
  {
    const float x = (l*l) / (2.0f * r);
    const float sign = (seg->type == TR_LFT) ? -1.0f : 1.0f;
    const float y = sign * sqrt(r*r - (x-r)*(x-r));

    const float w = sw + (ew - sw) * l / seg->length;
    const float s = w / 2.0f;

    samples.push_back(cSample(x, y, s));
  }
  return samples;
}

