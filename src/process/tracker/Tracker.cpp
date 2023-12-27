#include "Tracker.h"
#include <iostream>

// constructor
Tracker::Tracker(uint32_t _m, uint32_t _n, uint32_t _nDelete, double _cpi, double _maxAccInit)
{
  m = _m;
  n = _n;
  nDelete = _nDelete;
  cpi = _cpi;
  maxAccInit = _maxAccInit;
  timestamp = 0;

  double resolutionAcc = 1 * (1/(cpi*cpi));
  uint16_t nAcc = (int)maxAccInit/resolutionAcc;
  for (int i = 0; i < 2*nAcc+1; i++)
  {
    accInit.push_back(resolutionAcc*(i-nAcc));
  }

  Track track{};
}

Tracker::~Tracker()
{
}

Track *Tracker::process(Detection *detection, uint64_t currentTime)
{
  timestamp = currentTime;
  doNotInitiate.clear();
  for (size_t i = 0; i < detection->get_nDetections(); i++)
  {
    doNotInitiate.push_back(false);
  }

  if (track.get_n() > 0)
  {
    update(detection, timestamp);
  }
  initiate(detection);

  return &track;
}

void Tracker::update(Detection *detection, uint64_t current)
{
  std::vector<double> delay = detection->get_delay();
  std::vector<double> doppler = detection->get_doppler();
  std::vector<double> snr = detection->get_snr();

  // init
  double delayPredict, dopplerPredict;
  double acc;
  uint32_t nRemove = 0;
  std::string state;

  // get time between detections
  double T = (double) current - timestamp;

  // loop over each track
  for (int i = 0; i < track.get_n(); i++)
  {
    // predict next position
    Detection detectionCurrent = track.get_current(i);
    double delayTrack = detectionCurrent.get_delay().front();
    double dopplerTrack = detectionCurrent.get_doppler().front();
    acc = track.get_acceleration(i);
    delayPredict = delayTrack+(1/(2*3.14))*(dopplerTrack+0.5*acc*T*T);
    dopplerPredict = dopplerTrack+acc*T;
    Detection prediction(delayPredict, dopplerPredict, 0);

    // loop over detections to associate
    for (size_t j = 0; j < detection->get_nDetections(); j++)
    {
      // associate detections
      if (delay[j] > delayPredict-1 &&
        delay[j] < delayPredict+1 &&
        doppler[j] > dopplerPredict-(2/cpi) &&
        doppler[j] < dopplerPredict+(2/cpi))
      {
        Detection associated(delay[j], doppler[j], snr[j]);
        track.set_current(i, associated);
        track.set_acceleration(i, (doppler[j]-dopplerTrack)/T);
        track.set_nInactive(i, 0);
        doNotInitiate[j] = true;
        state = "ASSOCIATED";
        track.set_state(i, state);
        // check for track promotion
        track.promote(i, m, n);
        break;
      }
    }

    // update state if no detections associated
    track.set_current(i, prediction);
    if (track.get_state(i) == "ACTIVE")
    {
      state = "COASTING";
      track.set_state(i, state);
    }
    else
    {
      track.set_state(i, track.get_state(i));
    }
    track.set_nInactive(i, track.get_nInactive(i)+1);

    // remove if tentative or coasting too long
    if (track.get_nInactive(i) > nDelete)
    {
      track.remove(i+nRemove);
      nRemove++;
    }
  }
}

void Tracker::initiate(Detection *detection)
{  
  std::vector<double> delay = detection->get_delay();
  std::vector<double> doppler = detection->get_doppler();
  std::vector<double> snr = detection->get_snr();
  uint64_t index;

  // loop over new detections
  for (size_t i = 0; i < detection->get_nDetections(); i++)
  {
    // skip if detection used in update
    if (doNotInitiate.at(i))
    {
      continue;
    }
    // add tentative detection for each acc
    for (size_t j = 0; j < accInit.size(); j++)
    {
      Detection detectionCurrent(delay[i], doppler[i], snr[i]);
      index = track.add(detectionCurrent);
      track.set_acceleration(index, accInit[j]);
    }
  }
}