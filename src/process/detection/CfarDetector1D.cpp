#include "CfarDetector1D.h"
#include "data/Map.h"

#include <iostream>
#include <vector>
#include <cmath>

// constructor
CfarDetector1D::CfarDetector1D(double _pfa, int8_t _nGuard, int8_t _nTrain, int8_t _minDelay, double _minDoppler)
{
  // input
  pfa = _pfa;
  nGuard = _nGuard;
  nTrain = _nTrain;
  minDelay = _minDelay;
  minDoppler = _minDoppler;
}

CfarDetector1D::~CfarDetector1D()
{
}

std::unique_ptr<Detection> CfarDetector1D::process(Map<std::complex<double>> *x)
{ 
  int32_t nDelayBins = x->get_nCols();
  int32_t nDopplerBins = x->get_nRows();

  std::vector<std::complex<double>> mapRow;
  std::vector<double> mapRowSquare, mapRowSnr;

  // store detections temporarily
  std::vector<double> delay;
  std::vector<double> doppler;
  std::vector<double> snr;

  // loop over every cell
  for (int i = 0; i < nDopplerBins; i++)
  { 
    // skip if less than min Doppler
    if (std::abs(x->doppler[i]) < minDoppler)
    {
      continue;
    } 
    mapRow = x->get_row(i);
    for (int j = 0; j < nDelayBins; j++)
    {
      mapRowSquare.push_back((double) std::abs(mapRow[j]*mapRow[j]));
      mapRowSnr.push_back((double)10 * std::log10(std::abs(mapRow[j])) - x->noisePower);
    }
    for (int j = 0; j < nDelayBins; j++)
    {
      // skip if less than min delay
      if (x->delay[j] < minDelay)
      {
        continue;
      } 
      // get train cell indices
      std::vector<int> iTrain;
      for (int k = j-nGuard-nTrain; k < j-nGuard; k++)
      {
        if (k > 0 && k < nDelayBins)
        {
          iTrain.push_back(k);
        }
      }
      for (int k = j+nGuard+1; k < j+nGuard+nTrain+1; k++)
      {
        if (k >= 0 && k < nDelayBins)
        {
          iTrain.push_back(k);
        }
      }

      // compute threshold
      int nCells = iTrain.size();
      double alpha = nCells * (pow(pfa, -1.0 / nCells) - 1);
      double trainNoise = 0.0;
      for (int k = 0; k < nCells; k++)
      {
        trainNoise += mapRowSquare[iTrain[k]];
      }
      trainNoise /= nCells;
      double threshold = alpha * trainNoise;

      // detection if over threshold
      if (mapRowSquare[j] > threshold)
      {
        delay.push_back(j + x->delay[0]);
        doppler.push_back(x->doppler[i]);
        snr.push_back(mapRowSnr[j]);
      }
      iTrain.clear();
    }
    mapRowSquare.clear();
    mapRowSnr.clear();
  }

  // create detection
  return std::make_unique<Detection>(delay, doppler, snr);
}
