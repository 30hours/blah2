#ifndef WIENERHOPF_H
#define WIENERHOPF_H

#include <IqData.h>
#include <stdint.h>
#include <fftw3.h>
#include <armadillo>

class WienerHopf
{
private:
  int32_t delayMin;
  int32_t delayMax;
  uint32_t nBins;
  uint32_t nSamples;
  bool success;

  fftw_plan fftX, fftY, fftA, fftB, fftFiltX, fftFiltW, fftFilt;
  std::complex<double> *dataX, *dataY, *dataOutX, *dataOutY, *dataA, *dataB, *filtX, *filtW, *filt;
  std::deque<std::complex<double>> xData, yData;

  arma::cx_mat A;
  arma::cx_vec a, b, w;
  

public:
  WienerHopf(int32_t delayMin, int32_t delayMax, uint32_t nSamples);
  ~WienerHopf();
  bool process(IqData *x, IqData *y);
};

#endif