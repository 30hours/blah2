#include "Interpolate.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <stdint.h>
#include <algorithm>

// constructor
Interpolate::Interpolate(bool _doDelay, bool _doDoppler)
{
  // input
  doDelay = _doDelay;
  doDoppler = _doDoppler;
}

Interpolate::~Interpolate()
{
}

std::unique_ptr<Detection> Interpolate::process(Detection *x, Map<std::complex<double>> *y)
{ 
  // store detections temporarily
  std::vector<double> delay, doppler, snr;
  delay = x->get_delay();
  doppler = x->get_doppler();
  snr = x->get_snr();

  // interpolate data
  double intDelay, intDoppler, intSnrDelay, intSnrDoppler, intSnr[3];
  std::vector<double> delay2, doppler2, snr2;
  std::deque<int> indexDelay = y->delay;
  std::deque<double> indexDoppler = y->doppler;

  // loop over every detection
  for (size_t i = 0; i < snr.size(); i++)
  {
    // initialise interpolated values for bool flags
    intDelay = delay[i];
    intDoppler = doppler[i];
    intSnrDelay = snr[i];
    intSnrDoppler = snr[i];
    // interpolate in delay
    if (doDelay)
    {
      // check not on boundary
      if (delay[i] == indexDelay[0] || delay[i] == indexDelay.back())
      {
        continue;
      }
      intSnr[0] = (double)10*std::log10(std::abs(y->data[y->doppler_hz_to_bin(doppler[i])][delay[i]-1-indexDelay[0]]))-y->noisePower;
      intSnr[1] = (double)10*std::log10(std::abs(y->data[y->doppler_hz_to_bin(doppler[i])][delay[i]-indexDelay[0]]))-y->noisePower;
      intSnr[2] = (double)10*std::log10(std::abs(y->data[y->doppler_hz_to_bin(doppler[i])][delay[i]+1-indexDelay[0]]))-y->noisePower;
      // check detection has peak SNR of neighbours
      if (intSnr[1] < intSnr[0] || intSnr[1] < intSnr[2])
      {
          std::cout << "Detection dropped (SNR of peak lower)" << std::endl;
          continue;
      }
      intDelay = (intSnr[0]-intSnr[2])/(2*(intSnr[0]-(2*intSnr[1])+intSnr[2]));
      intSnrDelay = intSnr[1] - (((intSnr[0]-intSnr[2])*intDelay)/4);
      intDelay = delay[i] + intDelay;
    }
    // interpolate in Doppler
    if (doDoppler)
    {
      // check not on boundary
      if (doppler[i] == indexDoppler[0] || doppler[i] == indexDoppler.back())
      {
        continue;
      }
      intSnr[0] = (double)10*std::log10(std::abs(y->data[y->doppler_hz_to_bin(doppler[i])-1][delay[i]-indexDelay[0]]))-y->noisePower;
      intSnr[1] = (double)10*std::log10(std::abs(y->data[y->doppler_hz_to_bin(doppler[i])][delay[i]-indexDelay[0]]))-y->noisePower;
      intSnr[2] = (double)10*std::log10(std::abs(y->data[y->doppler_hz_to_bin(doppler[i])+1][delay[i]-indexDelay[0]]))-y->noisePower;
      // check detection has peak SNR of neighbours
      if (intSnr[1] < intSnr[0] || intSnr[1] < intSnr[2])
      {
          continue;
      }
      intDoppler = (intSnr[0]-intSnr[2])/(2*(intSnr[0]-(2*intSnr[1])+intSnr[2]));
      intSnrDelay = intSnr[1] - (((intSnr[0]-intSnr[2])*intDoppler)/4);
      intDoppler = doppler[i] + ((indexDoppler[1]-indexDoppler[0])*intDoppler);
    }
    // store interpolated detections
    delay2.push_back(intDelay);
    doppler2.push_back(intDoppler);
    snr2.push_back(std::max(std::max(intSnrDelay, intSnrDoppler), snr[i]));
  }

  // create detection
  return std::make_unique<Detection>(delay2, doppler2, snr2);
}
