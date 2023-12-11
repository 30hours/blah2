#include "SpectrumAnalyser.h"
#include <complex>
#include <iostream>
#include <deque>
#include <vector>
#include <math.h>

// constructor
SpectrumAnalyser::SpectrumAnalyser(uint32_t _n, double _bandwidth)
{
  // input
  n = _n;
  bandwidth = _bandwidth;

  // compute nfft
  decimation = n/bandwidth;
  nfft = n/decimation;

  // compute FFTW plans in constructor
  dataX = new std::complex<double>[nfft];
  fftX = fftw_plan_dft_1d(nfft, reinterpret_cast<fftw_complex *>(dataX),
                           reinterpret_cast<fftw_complex *>(dataX), FFTW_FORWARD, FFTW_ESTIMATE);
}

SpectrumAnalyser::~SpectrumAnalyser()
{
  fftw_destroy_plan(fftX);
}

void SpectrumAnalyser::process(IqData *x)
{  
  // decimate
  int16_t iData = 0;
  std::deque<std::complex<double>> data = x->get_data();
  for (int i = 0; i < x->get_length(); i+=decimation)
  {
    dataX[iData] = data[i];
    iData++;
  }

  fftw_execute(fftX);

  // update spectrum
  std::vector<std::complex<double>> spectrum;
  for (int i = 0; i < nfft; i++)
  {
    spectrum.push_back(dataX[i]);
  }
  x->update_spectrum(spectrum);

  // update frequency

  return;
}
