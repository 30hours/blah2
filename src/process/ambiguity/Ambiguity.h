/// @file Ambiguity.h
/// @class Ambiguity
/// @brief A class to implement a ambiguity map processing.
/// @details Implements a the batches algorithm as described in Principles of Modern Radar, Volume II, Chapter 17.
/// See Fundamentals of Radar Signal Processing (Richards) for more on the pulse-Doppler processing method.
/// @author 30hours
/// @todo Ambiguity maps are still offset by 1 bin.

#pragma once

#include <IqData.h>
#include <Map.h>
#include <HammingNumber.h>
#include <stdint.h>
#include <fftw3.h>
#include <memory>

class Ambiguity
{

public:

  using Complex = std::complex<double>;

  struct PerformanceStats {
    double process_time_ms{0};
    double range_fft_time_ms{0};
    double doppler_fft_time_ms{0};
  };


  /// @brief Constructor.
  /// @param delayMin Minimum delay (bins).
  /// @param delayMax Maximum delay (bins).
  /// @param dopplerMin Minimum Doppler (Hz).
  /// @param dopplerMax Maximum Doppler (Hz).
  /// @param fs Sampling frequency (Hz).
  /// @param n Number of samples.
  /// @param roundHamming Round the correlation FFT length to a Hamming number for performance
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

  double doppler_middle() const { return dopplerMiddle_; }

  uint16_t delay_bin_count() const { return nDelayBins_; }

  uint16_t doppler_bin_count() const { return nDopplerBins_; }

  uint16_t corr_samples_per_pulse() const { return nCorr_; }

  double cpi_length_seconds() const { return cpi_; }

  uint32_t fft_bin_count() const { return nfft_; }

  PerformanceStats get_latest_performance() const { return latest_performance_; }
private:
  /// @brief Minimum delay (bins).
  int32_t delayMin_;

  /// @brief Maximum delay (bins).
  int32_t delayMax_;

  /// @brief Minimum Doppler (Hz).
  int32_t dopplerMin_;

  /// @brief Maximum Doppler (Hz).
  int32_t dopplerMax_;

  /// @brief Sampling frequency (Hz).
  uint32_t fs_;

  /// @brief Number of samples.
  uint32_t nSamples_;

  /// @brief Center of Doppler bins (Hz).
  double dopplerMiddle_;

  /// @brief Number of delay bins.
  uint16_t nDelayBins_;

  /// @brief Number of Doppler bins.
  uint16_t nDopplerBins_;

  /// @brief Number of correlation samples per pulse.
  uint16_t nCorr_;

  /// @brief True CPI time (s).
  double cpi_;

  /// @brief FFTW plans for ambiguity processing.
  fftw_plan fftXi_;
  fftw_plan fftYi_;
  fftw_plan fftZi_;
  fftw_plan fftDoppler_;

  /// @brief FFTW storage for ambiguity processing.
  /// @{
  std::vector<Complex> dataXi_;
  std::vector<Complex> dataYi_;
  std::vector<Complex> dataZi_;
  std::vector<Complex> dataCorr_;
  std::vector<Complex> dataDoppler_;
  /// @}

  /// @brief Number of samples to perform FFT per pulse.
  uint32_t nfft_;

  /// @brief Vector storage for ambiguity processing
  /// @{
  std::vector<Complex> corr_;
  std::vector<Complex> delayProfile_;
  /// @}

  /// @brief Map to store result.
  std::unique_ptr<Map<Complex>> map_;

  PerformanceStats latest_performance_;
};

std::ostream& operator<<(std::ostream& str, const Ambiguity::PerformanceStats& stats);