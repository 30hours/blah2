/// @file Interpolate.h
/// @class Interpolate
/// @brief A class to interpolate detection data using a quadratic curve.
/// @details Interpolate in delay and Doppler. If 2 points either side have a higher SNR, then remove detection.
/// References:
/// - https://ccrma.stanford.edu/~jos/sasp/Quadratic_Interpolation_Spectral_Peaks.html
/// - Fundamentals of Signal Processing (2nd), Richards, Section 5.3.6
/// @author 30hours
/// @todo Should I remove the detection pointer? Also on Centroid.

#ifndef INTERPOLATE_H
#define INTERPOLATE_H

#include "data/Map.h"
#include "data/Detection.h"

#include <memory>

class Interpolate
{
private:
  /// @brief True if interpolating over delay.
  bool doDelay;

  /// @brief True if interpolating over Doppler.
  bool doDoppler;

  /// @brief Pointer to detection data to store result.
  Detection *detection;

public:
  /// @brief Constructor.
  /// @param doDelay True if interpolating over delay.
  /// @param doDoppler True if interpolating over Doppler.
  /// @return The object.
  Interpolate(bool doDelay, bool doDoppler);

  /// @brief Destructor.
  /// @return Void.
  ~Interpolate();

  /// @brief Implement the 1D CFAR detector.
  /// @param x Detections from the 1D CFAR detector.
  /// @return Interpolated detections.
  std::unique_ptr<Detection> process(Detection *x, Map<std::complex<double>> *y);
};

#endif
