#ifndef AMBIGUITY_H
#define AMBIGUITY_H

#include <IqData.h>
#include <Map.h>
#include <stdint.h>
#include <fftw3.h>

class Ambiguity
{
private:
  int32_t delayMin;
  int32_t delayMax;
  int32_t dopplerMin;
  int32_t dopplerMax;
  uint32_t fs;
  uint32_t n;
  double dopplerMiddle;

  uint16_t nDelayBins;
  uint16_t nDopplerBins;
  uint16_t nCorr;
  double cpi;

  fftw_plan fftXi, fftYi, fftZi, fftDoppler;
  std::complex<double> *dataXi, *dataYi, *dataZi, *dataCorr, *dataDoppler;
  uint32_t nfft;

  std::vector<std::complex<double>> corr, delayProfile;

  Map<std::complex<double>> *map;

public:
  Ambiguity(int32_t delayMin, int32_t delayMax, int32_t dopplerMin, int32_t dopplerMax, uint32_t fs, uint32_t n);
  ~Ambiguity();
  Map<std::complex<double>> *process(IqData *x, IqData *y);
};

#endif