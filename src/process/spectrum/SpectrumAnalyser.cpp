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
  nSpectrum = n/decimation;
  nfft = nSpectrum*decimation;

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
  // load data and FFT
  uint32_t i;
  std::deque<std::complex<double>> data = x->get_data();
  for (i = 0; i < nfft; i++)
  {
    dataX[i] = data[i];
  }
  fftw_execute(fftX);

  // fftshift
  std::vector<std::complex<double>> fftshift;
  for (i = 0; i < nfft; i++)
  {
    fftshift.push_back(dataX[(i + int(nfft / 2) + 1) % nfft]);
  }
  
  // decimate
  std::vector<std::complex<double>> spectrum;
  for (i = 0; i < nfft; i+=decimation)
  {
    spectrum.push_back(fftshift[i]);
  }
  x->update_spectrum(spectrum);

  // update frequency
  std::vector<double> frequency;
  double offset = 0;
  if (decimation % 2 == 0)
  {
    offset = bandwidth/2;
  }
  for (i = -nSpectrum/2; i < nSpectrum/2; i++)
  {
    frequency.push_back(((i*bandwidth)+offset+204640000)/1000);
  }
  x->update_frequency(frequency);

  return;
}
