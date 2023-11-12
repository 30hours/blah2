#include "CfarDetector1D.h"
#include <iostream>
#include <vector>
#include <cmath>

// constructor
CfarDetector1D::CfarDetector1D(double _pfa, int8_t _nGuard, int8_t _nTrain)
{
  // input
  pfa = _pfa;
  nGuard = _nGuard;
  nTrain = _nTrain;
}

CfarDetector1D::~CfarDetector1D()
{
}

Detection *CfarDetector1D::process(Map<std::complex<double>> *x)
{
  std::vector<std::vector<double>> dataSnr;
  std::vector<std::vector<double>> dataSquare;

  // compute square of Map
  for (int i = 0; i < x->data.size(); i++)
  {
    std::vector<double> dataSnrRow;
    std::vector<double> dataSquareRow;
    for (int j = 0; j < x->data[i].size(); j++)
    {
      dataSnrRow.push_back(10 * log10(std::abs(x->data[i][j])) - x->noisePower);
      dataSquareRow.push_back(std::abs(x->data[i][j]) * std::abs(x->data[i][j]));
    }
    dataSnr.push_back(dataSnrRow);
    dataSquare.push_back(dataSquareRow);
  }

  int32_t nDelayBins = x->get_nRows();
  int32_t nDopplerBins = x->get_nCols();

  // store detections temporarily
  std::vector<double> delay;
  std::vector<double> doppler;
  std::vector<double> snr;

  // loop over every cell
  for (int iDelay = 0; iDelay < nDelayBins; iDelay++)
  {
    for (int iDoppler = 0; iDoppler < nDopplerBins; iDoppler++)
    {

      // get train cell indices
      std::vector<int> iTrain;
      for (int k = iDelay - nGuard - nTrain; k <= iDelay + nGuard + nTrain; ++k)
      {
        if (k >= 1 && k <= nDelayBins)
        {
          iTrain.push_back(k);
        }
      }

      // compute threshold
      int nCells = iTrain.size();
      double alpha = nCells * (pow(pfa, -1.0 / nCells) - 1);
      double trainNoise = 0.0;
      for (int k = 0; k < nCells; ++k)
      {
        trainNoise += dataSquare[iDoppler][iTrain[k] - 1];
      }
      trainNoise /= nCells;
      double threshold = alpha * trainNoise;

      // detection if over threshold
      if (dataSquare[iDoppler][iDelay] > threshold)
      {
        delay.push_back(iDelay + x->delay[0] - 1);
        doppler.push_back(x->doppler[iDoppler]);
        //snr.push_back(dataSnr[iDoppler][iDelay]);
        snr.push_back(-1);
      }
    }
  }

  // create detection
  Detection *detection = new Detection(delay, doppler, snr);

  return detection;
}
