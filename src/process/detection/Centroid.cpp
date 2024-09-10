#include "Centroid.h"
#include <iostream>
#include <vector>
#include <cmath>

// constructor
Centroid::Centroid(uint16_t _nDelay, uint16_t _nDoppler, double _resolutionDoppler)
{
  // input
  nDelay = _nDelay;
  nDoppler = _nDoppler;
  resolutionDoppler = _resolutionDoppler;
}

Centroid::~Centroid()
{
}

std::unique_ptr<Detection> Centroid::process(Detection *x)
{ 
  // store detections temporarily
  std::vector<double> delay, doppler, snr;
  delay = x->get_delay();
  doppler = x->get_doppler();
  snr = x->get_snr();

  // centroid data
  uint16_t delayMin, delayMax;
  double dopplerMin, dopplerMax;
  bool isCentroid;
  std::vector<double> delay2, doppler2, snr2;

  // loop over every detection
  for (size_t i = 0; i < snr.size(); i++)
  {
    delayMin = (int)(delay[i]) - nDelay;
    delayMax = (int)(delay[i]) + nDelay;
    dopplerMin = doppler[i] - (nDoppler * resolutionDoppler);
    dopplerMax = doppler[i] + (nDoppler * resolutionDoppler);
    isCentroid = true;
    
    // find detections to keep
    for (size_t j = 0; j < snr.size(); j++)
    {
      // skip same detection
      if (j == i)
      {
        continue;
      }
      // search detections close by
      if (delay[j] > delayMin && delay[j] < delayMax &&
        doppler[j] > dopplerMin && doppler[j] < dopplerMax)
        {
          // remove if SNR is lower
          if (snr[i] < snr[j])
          {
            isCentroid = false;
            break;
          }
        }
    }
    // store centroided detections
    if (isCentroid)
    {
      delay2.push_back(delay[i]);
      doppler2.push_back(doppler[i]);
      snr2.push_back(snr[i]);  
    }
  }

  // create detection
  return std::make_unique<Detection>(delay2, doppler2, snr2);
}
