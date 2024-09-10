#include "Tracker.h"
#include <iostream>

// constructor
Tracker::Tracker(uint32_t _m, uint32_t _n, uint32_t _nDelete, 
  double _cpi, double _maxAccInit, double _rangeRes, double _lambda)
{
  m = _m;
  n = _n;
  nDelete = _nDelete;
  cpi = _cpi;
  maxAccInit = _maxAccInit;
  timestamp = 0;
  rangeRes = _rangeRes;
  lambda = _lambda;

  double resolutionAcc = 1/(cpi*cpi);
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

std::unique_ptr<Track> Tracker::process(Detection *detection, uint64_t currentTime)
{
  doNotInitiate.clear();
  for (size_t i = 0; i < detection->get_nDetections(); i++)
  {
    doNotInitiate.push_back(false);
  }

  if (track.get_n() > 0)
  {
    update(detection, currentTime);
  }
  else
  {
    timestamp = currentTime;
  }
  initiate(detection);

  return std::make_unique<Track>(track);
}

void Tracker::update(Detection *detection, uint64_t current)
{
  std::vector<double> delay = detection->get_delay();
  std::vector<double> doppler = detection->get_doppler();
  std::vector<double> snr = detection->get_snr();

  // init
  double delayPredict = 0.0;
  double dopplerPredict = 0.0;
  double acc = 0.0;
  uint32_t nRemove = 0;
  std::string state;

  // get time between detections
  double T = ((double)(current - timestamp))/1000;
  timestamp = current;

  // loop over each track
  for (uint64_t i = 0; i < track.get_n(); i++)
  {
    // predict next position
    Detection detectionCurrent = track.get_current(i);
    acc = track.get_acceleration(i);
    Detection prediction = predict(detectionCurrent, acc, T);
    
    // loop over detections to associate
    for (size_t j = 0; j < detection->get_nDetections(); j++)
    {
      // associate detections
      if (delay[j] > delayPredict-1 &&
        delay[j] < delayPredict+1 &&
        doppler[j] > dopplerPredict-1*(1/cpi) &&
        doppler[j] < dopplerPredict+1*(1/cpi))
      {
        Detection associated(delay[j], doppler[j], snr[j]);
        track.set_current(i, associated);
        track.set_acceleration(i, (doppler[j]-detectionCurrent.get_doppler().front())/T);
        track.set_nInactive(i, 0);
        doNotInitiate[j] = true;
        state = "ASSOCIATED";
        track.set_state(i, state);
        // promote track if passes threshold
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
    else if (track.get_state(i) == "ASSOCIATED")
    {
      state = "TENTATIVE";
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
      track.remove(i-nRemove);
      nRemove++;
    }
  }
}

Detection Tracker::predict(Detection current, double acc, double T)
{
  double delayTrack = current.get_delay().front();
  double dopplerTrack = current.get_doppler().front();
  double delayPredict = delayTrack+((dopplerTrack*T*lambda)+
    (0.5*acc*T*T))/rangeRes;
  double dopplerPredict = dopplerTrack+(acc*T);
  Detection prediction(delayPredict, dopplerPredict, 0);
  return prediction;
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