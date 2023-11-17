#include "Ambiguity.h"
#include <complex>
#include <iostream>
#include <deque>
#include <vector>
#include <math.h>

// constructor
Ambiguity::Ambiguity(int32_t _delayMin, int32_t _delayMax, int32_t _dopplerMin, int32_t _dopplerMax, uint32_t _fs, uint32_t _n)
{
  // input
  delayMin = _delayMin;
  delayMax = _delayMax;
  dopplerMin = _dopplerMin;
  dopplerMax = _dopplerMax;
  fs = _fs;
  n = _n;

  // delay calculations
  std::deque<int> delay;
  nDelayBins = delayMax - delayMin + 1;
  for (int i = 0; i < nDelayBins; i++)
  {
    delay.push_back(delayMin + i);
  }

  // doppler calculations
  std::deque<double> doppler;
  double resolutionDoppler = (double)1 / ((double)n / (double)fs);
  dopplerMiddle = (dopplerMin + dopplerMax) / 2;
  doppler.push_back(dopplerMiddle);
  int i = 1;
  while (dopplerMiddle + (i * resolutionDoppler) <= dopplerMax)
  {
    doppler.push_back(dopplerMiddle + (i * resolutionDoppler));
    doppler.push_front(dopplerMiddle - (i * resolutionDoppler));
    i++;
  }
  nDopplerBins = doppler.size();

  // batches constants
  nCorr = (int)(n / nDopplerBins);
  cpi = ((double)nCorr * (double)nDopplerBins) / fs;

  // update doppler bins to true cpi time
  resolutionDoppler = 1 / cpi;
  doppler.clear();
  doppler.push_back(dopplerMiddle);
  i = 1;
  while (doppler.size() < nDopplerBins)
  {
    doppler.push_back(dopplerMiddle + (i * resolutionDoppler));
    doppler.push_front(dopplerMiddle - (i * resolutionDoppler));
    i++;
  }

  // other setup
  nfft = (2 * nCorr) - 1;
  dataCorr = new std::complex<double>[2 * nDelayBins + 1];

  // compute FFTW plans in constructor
  dataXi = new std::complex<double>[nfft];
  dataYi = new std::complex<double>[nfft];
  dataZi = new std::complex<double>[nfft];
  dataDoppler = new std::complex<double>[nfft];
  fftXi = fftw_plan_dft_1d(nfft, reinterpret_cast<fftw_complex *>(dataXi),
                           reinterpret_cast<fftw_complex *>(dataXi), FFTW_FORWARD, FFTW_ESTIMATE);
  fftYi = fftw_plan_dft_1d(nfft, reinterpret_cast<fftw_complex *>(dataYi),
                           reinterpret_cast<fftw_complex *>(dataYi), FFTW_FORWARD, FFTW_ESTIMATE);
  fftZi = fftw_plan_dft_1d(nfft, reinterpret_cast<fftw_complex *>(dataZi),
                           reinterpret_cast<fftw_complex *>(dataZi), FFTW_BACKWARD, FFTW_ESTIMATE);
  fftDoppler = fftw_plan_dft_1d(nDopplerBins, reinterpret_cast<fftw_complex *>(dataDoppler),
                                reinterpret_cast<fftw_complex *>(dataDoppler), FFTW_FORWARD, FFTW_ESTIMATE);

  // create ambiguity map
  map = new Map<std::complex<double>>(nDopplerBins, nDelayBins);

  // store map parameters
  for (int i = 0; i < nDelayBins; i++)
  {
    map->delay.push_back(delay[i]);
  }
  for (int i = 0; i < nDopplerBins; i++)
  {
    map->doppler.push_back(doppler[i]);
  }
}

Ambiguity::~Ambiguity()
{
  fftw_destroy_plan(fftXi);
  fftw_destroy_plan(fftYi);
  fftw_destroy_plan(fftZi);
  fftw_destroy_plan(fftDoppler);
}

Map<std::complex<double>> *Ambiguity::process(IqData *x, IqData *y)
{
  // shift reference if not 0 centered
  if (dopplerMiddle != 0)
  {
    std::complex<double> j = {0, 1};
    for (int i = 0; i < x->get_length(); i++)
    {
      x->push_back(x->pop_front() * std::exp(1.0 * j * 2.0 * M_PI * dopplerMiddle * ((double)i / fs)));
    }
  }

  // range processing
  for (int i = 0; i < nDopplerBins; i++)
  {
    for (int j = 0; j < nCorr; j++)
    {
      dataXi[j] = x->pop_front();
      dataYi[j] = y->pop_front();
    }

    for (int j = nCorr; j < nfft; j++)
    {
      dataXi[j] = {0, 0};
      dataYi[j] = {0, 0};
    }

    fftw_execute(fftXi);
    fftw_execute(fftYi);

    // compute correlation
    for (int j = 0; j < nfft; j++)
    {
      dataZi[j] = (dataYi[j] * std::conj(dataXi[j])) / (double)nfft;
    }

    fftw_execute(fftZi);

    // extract center of corr
    for (int j = 0; j < nDelayBins; j++)
    {
      dataCorr[j] = dataZi[nfft - nDelayBins + j];
    }
    for (int j = 0; j < nDelayBins + 1; j++)
    {
      dataCorr[j + nDelayBins] = dataZi[j];
    }

    // cast from std::complex to std::vector
    corr.clear();
    for (int j = 0; j < nDelayBins; j++)
    {
      corr.push_back(dataCorr[nDelayBins + delayMin + j - 1 + 1]);
    }

    map->set_row(i, corr);
  }

  // doppler processing
  for (int i = 0; i < nDelayBins; i++)
  {
    delayProfile = map->get_col(i);
    for (int j = 0; j < nDopplerBins; j++)
    {
      dataDoppler[j] = {delayProfile[j].real(), delayProfile[j].imag()};
    }

    fftw_execute(fftDoppler);

    corr.clear();
    for (int j = 0; j < nDopplerBins; j++)
    {
      corr.push_back(dataDoppler[(j + int(nDopplerBins / 2) + 1) % nDopplerBins]);
    }

    map->set_col(i, corr);
  }

  return map;
}
