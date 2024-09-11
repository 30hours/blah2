/// @file Ambiguity.h
/// @class Ambiguity
/// @brief A class to implement a ambiguity map processing.
/// @details Implements a the batches algorithm as described in Principles of Modern Radar, Volume II, Chapter 17.
/// See Fundamentals of Radar Signal Processing (Richards) for more on the pulse-Doppler processing method.
/// @author 30hours
/// @todo Ambiguity maps are still offset by 1 bin.
/// @todo Write a performance test for hamming assisted ambiguity processing.
/// @todo If delayMin > delayMax = trouble, what's the exception policy?

#include "data/IqData.h"
#include "data/Map.h"
#include "process/meta/HammingNumber.h"
#include <stdint.h>
#include <fftw3.h>
#include <memory>

class Ambiguity
{

public:

  using Complex = std::complex<double>;

  /// @brief Constructor.
  /// @param delayMin Minimum delay (bins).
  /// @param delayMax Maximum delay (bins).
  /// @param dopplerMin Minimum Doppler (Hz).
  /// @param dopplerMax Maximum Doppler (Hz).
  /// @param fs Sampling frequency (Hz).
  /// @param n Number of samples.
  /// @param roundHamming Round the correlation FFT length to a Hamming number for performance.
  /// @return The object.
  Ambiguity(int32_t delayMin, int32_t delayMax, int32_t dopplerMin, int32_t dopplerMax, uint32_t fs, uint32_t n, bool roundHamming = false);

  /// @brief Destructor.
  /// @return Void.
  ~Ambiguity();

  /// @brief Implement the ambiguity processor.
  /// @param x Reference samples.
  /// @param y Surveillance samples.
  /// @return Ambiguity map data of IQ samples.
  Map<Complex> *process(IqData *x, IqData *y);

  double get_doppler_middle() const;

  uint16_t get_n_delay_bins() const;

  uint16_t get_n_doppler_bins() const;

  uint16_t get_n_corr() const;

  double get_cpi() const;

  uint32_t get_nfft() const;

  uint32_t get_n_samples() const;

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
  uint32_t nSamples;

  /// @brief Number of delay bins.
  uint16_t nDelayBins;

  /// @brief Center of Doppler bins (Hz).
  double dopplerMiddle;

  /// @brief Number of Doppler bins.
  uint16_t nDopplerBins;

  /// @brief Number of correlation samples per pulse.
  uint16_t nCorr;

  /// @brief True CPI time (s).
  double cpi;

  /// @brief FFTW plans for ambiguity processing.
  fftw_plan fftXi;
  fftw_plan fftYi;
  fftw_plan fftZi;
  fftw_plan fftDoppler;

  /// @brief FFTW storage for ambiguity processing.
  /// @{
  std::vector<Complex> dataXi;
  std::vector<Complex> dataYi;
  std::vector<Complex> dataZi;
  std::vector<Complex> dataCorr;
  std::vector<Complex> dataDoppler;
  /// @}

  /// @brief Number of samples to perform FFT per pulse.
  uint32_t nfft;

  /// @brief Vector storage for ambiguity processing
  /// @{
  std::vector<Complex> corr;
  std::vector<Complex> delayProfile;
  /// @}

  /// @brief Map to store result.
  std::unique_ptr<Map<Complex>> map;

};
