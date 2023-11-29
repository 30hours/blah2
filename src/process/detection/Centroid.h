/// @file Centroid.h
/// @class Centroid
/// @brief A class to remove duplicate target detections.
/// @details If detection SNR is larger than neighbours, then remove.
/// @author 30hours
/// @todo Still a bug where sometimes 2 consecutive range detections get through.

#ifndef CENTROID_H
#define CENTROID_H

#include <Detection.h>
#include <stdint.h>

class Centroid
{
private:
  /// @brief Number of delay bins to check.
  int8_t nDelay;

  /// @brief Number of Doppler bins to check.
  int8_t nDoppler;

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
  Centroid(int8_t nDelay, int8_t nDoppler, double resolutionDoppler);

  /// @brief Destructor.
  /// @return Void.
  ~Centroid();

  /// @brief Implement the 1D CFAR detector.
  /// @param x Detections from the 1D CFAR detector.
  /// @return Centroided detections.
  Detection *process(Detection *x);
};

#endif
