/// @file WienerHopf.h
/// @class WienerHopf
/// @brief A class to implement a Wiener-Hopf clutter filter.
/// @details Implements a <a href="https://en.wikipedia.org/wiki/Wiener_filter#Finite_impulse_response_Wiener_filter_for_discrete_series">Wiener-Hopf filter</a>.
/// Uses <a href="https://en.wikipedia.org/wiki/Cholesky_decomposition">Cholesky decomposition</a> to speed up matrix inversion, as the Toeplitz matrix is positive-definite and Hermitian.
/// @author 30hours
/// @todo Fix the segmentation fault from clutter filter numerical instability.

#ifndef WIENERHOPF_H
#define WIENERHOPF_H

#include <IqData.h>
#include <stdint.h>
#include <fftw3.h>
#include <armadillo>

class WienerHopf
{
private:
  /// @brief Minimum clutter filter delay (bins).
  int32_t delayMin;

  /// @brief Maximum clutter filter delay (bins).
  int32_t delayMax;

  /// @brief Number of bins (delayMax - delayMin + 1).
  uint32_t nBins;

  /// @brief Number of samples per CPI.
  uint32_t nSamples;

  /// @brief True if clutter filter processing is successful.
  bool success;

  /// @brief FFTW plans for clutter filter processing.
  /// @{
  fftw_plan fftX, fftY, fftA, fftB, fftFiltX, fftFiltW, fftFilt;
  /// @}

  /// @brief FFTW storage for clutter filter processing.
  /// @{
  std::complex<double> *dataX, *dataY, *dataOutX, *dataOutY, *dataA, *dataB, *filtX, *filtW, *filt;
  /// @}

  /// @brief Deque storage for clutter filter processing.
  /// @{
  std::deque<std::complex<double>> xData, yData;
  /// @}

  /// @brief Autocorrelation toeplitz matrix.
  arma::cx_mat A;

  /// @brief Autocorrelation vector.
  arma::cx_vec a;

  /// @brief Cross-correlation vector.
  arma::cx_vec b;

  /// @brief Weights vector.
  arma::cx_vec w;

public:
  /// @brief Constructor.
  /// @param delayMin Minimum clutter filter delay (bins).
  /// @param delayMax Maximum clutter filter delay (bins).
  /// @param nSamples Number of samples per CPI.
  /// @return The object.
  WienerHopf(int32_t delayMin, int32_t delayMax, uint32_t nSamples);

  /// @brief Destructor.
  /// @return Void.
  ~WienerHopf();

  /// @brief Implement the clutter filter.
  /// @param x Reference samples.
  /// @param y Surveillance samples.
  /// @return True if clutter filter successful.
  bool process(IqData *x, IqData *y);
};

#endif