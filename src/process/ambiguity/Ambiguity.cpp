#include "Ambiguity.h"
#include <complex>
#include <iostream>
#include <deque>
#include <vector>
#include <numeric>
#include <math.h>
#include <chrono>

// constructor
Ambiguity::Ambiguity(int32_t _delayMin, int32_t _delayMax, 
  int32_t _dopplerMin, int32_t _dopplerMax, uint32_t _fs, 
  uint32_t _n, bool _roundHamming)
{
  // init
  delayMin = _delayMin;
  delayMax = _delayMax;
  dopplerMin = _dopplerMin;
  dopplerMax = _dopplerMax;
  fs = _fs;
  nSamples = _n;
  nDelayBins = static_cast<uint16_t>(_delayMax - _delayMin + 1);
  dopplerMiddle = (_dopplerMin + _dopplerMax) / 2.0;
  
  // doppler calculations
  std::deque<double> doppler;
  double resolutionDoppler = 1.0 / (static_cast<double>(_n) / static_cast<double>(_fs));
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
  nCorr = _n / nDopplerBins;
  cpi = (static_cast<double>(nCorr) * nDopplerBins) / fs;

  // update doppler bins to true cpi time
  resolutionDoppler = 1.0 / cpi;

  // create ambiguity map
  map = std::make_unique<Map<Complex>>(nDopplerBins, nDelayBins);

  // delay calculations
  map->delay.resize(nDelayBins);
  std::iota(map->delay.begin(), map->delay.end(), delayMin);

  map->doppler.push_front(dopplerMiddle);
  i = 1;
  while (map->doppler.size() < nDopplerBins)
  {
    map->doppler.push_back(dopplerMiddle + (i * resolutionDoppler));
    map->doppler.push_front(dopplerMiddle - (i * resolutionDoppler));
    i++;
  }

  // other setup
  nfft = 2 * nCorr - 1;
  if (_roundHamming) {
    nfft = next_hamming(nfft);
  }
  dataCorr.resize(2 * nDelayBins + 1);

  // compute FFTW plans in constructor
  dataXi.resize(nfft);
  dataYi.resize(nfft);
  dataZi.resize(nfft);
  dataDoppler.resize(nfft);
  fftXi = fftw_plan_dft_1d(nfft, reinterpret_cast<fftw_complex *>(dataXi.data()),
                           reinterpret_cast<fftw_complex *>(dataXi.data()), FFTW_FORWARD, FFTW_ESTIMATE);
  fftYi = fftw_plan_dft_1d(nfft, reinterpret_cast<fftw_complex *>(dataYi.data()),
                           reinterpret_cast<fftw_complex *>(dataYi.data()), FFTW_FORWARD, FFTW_ESTIMATE);
  fftZi = fftw_plan_dft_1d(nfft, reinterpret_cast<fftw_complex *>(dataZi.data()),
                           reinterpret_cast<fftw_complex *>(dataZi.data()), FFTW_BACKWARD, FFTW_ESTIMATE);
  fftDoppler = fftw_plan_dft_1d(nDopplerBins, reinterpret_cast<fftw_complex *>(dataDoppler.data()),
                                reinterpret_cast<fftw_complex *>(dataDoppler.data()), FFTW_FORWARD, FFTW_ESTIMATE);

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
    for (uint32_t i = 0; i < x->get_length(); i++)
    {
      x->push_back(x->pop_front() * std::exp(1.0 * j * 2.0 * M_PI * dopplerMiddle * ((double)i / fs)));
    }
  }

  // range processing
  nSamples = nDopplerBins * nCorr;
  for (uint16_t i = 0; i < nDopplerBins; i++)
  {
    for (uint16_t j = 0; j < nCorr; j++)
    {
      dataXi[j] = x->pop_front();
      dataYi[j] = y->pop_front();
    }

    for (uint16_t j = nCorr; j < nfft; j++)
    {
      dataXi[j] = {0, 0};
      dataYi[j] = {0, 0};
    }

    fftw_execute(fftXi);
    fftw_execute(fftYi);

    // compute correlation
    for (uint32_t j = 0; j < nfft; j++)
    {
      dataZi[j] = (dataYi[j] * std::conj(dataXi[j])) / (double)nfft;
    }

    fftw_execute(fftZi);

    // extract center of corr
    for (uint16_t j = 0; j < nDelayBins; j++)
    {
      dataCorr[j] = dataZi[nfft - nDelayBins + j];
    }
    for (uint16_t j = 0; j < nDelayBins + 1; j++)
    {
      dataCorr[j + nDelayBins] = dataZi[j];
    }

    // cast from std::complex to std::vector
    corr.clear();
    for (uint16_t j = 0; j < nDelayBins; j++)
    {
      corr.push_back(dataCorr[nDelayBins + delayMin + j - 1 + 1]);
    }

    map->set_row(i, corr);
  }

  // doppler processing
  for (uint16_t i = 0; i < nDelayBins; i++)
  {
    delayProfile = map->get_col(i);
    for (uint16_t j = 0; j < nDopplerBins; j++)
    {
      dataDoppler[j] = {delayProfile[j].real(), delayProfile[j].imag()};
    }

    fftw_execute(fftDoppler);

    corr.clear();
    for (uint16_t j = 0; j < nDopplerBins; j++)
    {
      corr.push_back(dataDoppler[(j + int(nDopplerBins / 2) + 1) % nDopplerBins]);
    }

    map->set_col(i, corr);
  }

  return map.get();
}

double Ambiguity::get_doppler_middle() const {
  return dopplerMiddle;
}

uint16_t Ambiguity::get_n_delay_bins() const {
  return nDelayBins;
}

uint16_t Ambiguity::get_n_doppler_bins() const {
  return nDopplerBins;
}

uint16_t Ambiguity::get_n_corr() const {
  return nCorr;
}

double Ambiguity::get_cpi() const {
  return cpi;
}

uint32_t Ambiguity::get_nfft() const {
  return nfft;
}

uint32_t Ambiguity::get_n_samples() const {
  return nSamples;
}