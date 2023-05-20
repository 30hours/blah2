/// @file Ambiguity.h
/// @class Ambiguity
/// @brief A class to implement a ambiguity map processing.
/// @details Implements a the batches algorithm as described in Principles of Modern Radar, Volume II, Chapter 17.
/// See Fundamentals of Radar Signal Processing (Richards) for more on the pulse-Doppler processing method.
/// @author 30hours
/// @todo Ambiguity maps are still offset by 1 bin.

#ifndef AMBIGUITY_H
#define AMBIGUITY_H

#include <IqData.h>
#include <Map.h>
#include <stdint.h>
#include <fftw3.h>

class Ambiguity
{
private:
  /// @brief Minimum delay (bins).
  int32_t delayMin;

  /// @brief Maximum delay (bins).
  int32_t delayMax;

  /// @brief Minimum Doppler (Hz).
  int32_t dopplerMin;

  /// @brief Maximum Doppler (Hz).
  int32_t dopplerMax;

  /// @brief Sampling frequency (Hz).
  uint32_t fs;

  /// @brief Number of samples.
  uint32_t n;

  /// @brief Center of Doppler bins (Hz).
  double dopplerMiddle;

  /// @brief Number of delay bins.
  uint16_t nDelayBins;

  /// @brief Number of Doppler bins.
  uint16_t nDopplerBins;

  /// @brief Number of correlation samples per pulse.
  uint16_t nCorr;

  /// @brief True CPI time (s).
  double cpi;

  /// @brief FFTW plans for ambiguity processing.
  /// @{
  fftw_plan fftXi, fftYi, fftZi, fftDoppler;
  /// @}

  /// @brief FFTW storage for ambiguity processing.
  /// @{
  std::complex<double> *dataXi, *dataYi, *dataZi, *dataCorr, *dataDoppler;
  /// @}

  /// @brief Number of samples to perform FFT per pulse.
  uint32_t nfft;

  /// @brief Vector storage for ambiguity processing
  /// @{
  std::vector<std::complex<double>> corr, delayProfile;
  /// @}

  /// @brief Pointer to map to store result.
  Map<std::complex<double>> *map;

public:
  /// @brief Constructor.
  /// @param delayMin Minimum delay (bins).
  /// @param delayMax Maximum delay (bins).
  /// @param dopplerMin Minimum Doppler (Hz).
  /// @param dopplerMax Maximum Doppler (Hz).
  /// @param fs Sampling frequency (Hz).
  /// @param n Number of samples.
  /// @return The object.
  Ambiguity(int32_t delayMin, int32_t delayMax, int32_t dopplerMin, int32_t dopplerMax, uint32_t fs, uint32_t n);

  /// @brief Destructor.
  /// @return Void.
  ~Ambiguity();

  /// @brief Implement the ambiguity processor.
  /// @param x Reference samples.
  /// @param y Surveillance samples.
  /// @return Ambiguity map data of IQ samples.
  Map<std::complex<double>> *process(IqData *x, IqData *y);
};

#endif