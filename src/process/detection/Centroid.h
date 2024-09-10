/// @file Centroid.h
/// @class Centroid
/// @brief A class to remove duplicate target detections.
/// @details If detection SNR is larger than neighbours, then remove.
/// @author 30hours

#ifndef CENTROID_H
#define CENTROID_H

#include "data/Detection.h"
#include <stdint.h>
#include <memory>

class Centroid
{
private:
  /// @brief Number of delay bins to check.
  uint16_t nDelay;

  /// @brief Number of Doppler bins to check.
  uint16_t nDoppler;

  /// @brief Doppler resolution to convert Hz to bins (Hz).
  double resolutionDoppler;

  /// @brief Pointer to detection data to store result.
  Detection *detection;

public:
  /// @brief Constructor.
  /// @param nDelay Number of delay bins to check.
  /// @param nDoppler Number of Doppler bins to check.
  /// @param resolutionDoppler Doppler resolution to convert Hz to bins (Hz).
  /// @return The object.
  Centroid(uint16_t nDelay, uint16_t nDoppler, double resolutionDoppler);

  /// @brief Destructor.
  /// @return Void.
  ~Centroid();

  /// @brief Implement the 1D CFAR detector.
  /// @param x Detections from the 1D CFAR detector.
  /// @return Centroided detections.
  std::unique_ptr<Detection> process(Detection *x);
};

#endif
