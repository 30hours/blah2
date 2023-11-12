/// @file Detection.h
/// @class Detection
/// @brief A class to store detection data.
/// @author 30hours

#ifndef DETECTION_H
#define DETECTION_H

#include <stdint.h>
#include <vector>
#include <complex>

class Detection
{
private:
  /// @brief Detections in delay (bins)
  std::vector<double> delay;

  /// @brief Detections in Doppler (Hz)
  std::vector<double> doppler;

  /// @brief Detections in SNR
  std::vector<double> snr;

public:
  /// @brief Constructor.
  /// @param delay Detections in delay (bins).
  /// @param doppler Detections in Doppler (Hz).
  /// @return The object.
  Detection(std::vector<double> delay, std::vector<double> doppler, std::vector<double> snr);

  /// @brief Get number of detections.
  /// @return Number of detections
  uint8_t get_nDetections();

  /// @brief Generate JSON of the detections and metadata.
  /// @return JSON string.
  std::string to_json();

  /// @brief Append the detections to a save file.
  /// @param json JSON string of detections and metadata.
  /// @param path Path of file to save.
  /// @return True is save is successful.
  bool save(std::string json, std::string path);
};

#endif
