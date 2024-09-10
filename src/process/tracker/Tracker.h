/// @file Tracker.h
/// @class Tracker
/// @brief A class to implement a bistatic tracker.
/// @details Key functions are update, initiate, smooth and remove.
/// @details Update before initiate to avoid duplicate tracks.
/// @author 30hours
/// @todo Add smoothing capability.
/// @todo Fix units up.
/// @todo I don't think I callback the true CPI time from ambiguity.

#ifndef TRACKER_H
#define TRACKER_H

#include "data/Detection.h"
#include "data/Track.h"

#include <stdint.h>
#include <memory>

class Tracker
{
private:
  /// @brief Track initiation constant for M of N detections.
  uint32_t m;

  /// @brief Track initiation constant for M of N detections.
  uint32_t n;

  /// @brief Number of missed predictions to delete a tentative track.
  uint32_t nDelete;

  /// @brief True CPI time for acceleration resolution(s).
  double cpi;

  /// @brief Maximum acceleration to initiate track (Hz/s).
  double maxAccInit;

  /// @brief Range resolution for kinematics equations (m).
  double rangeRes;

  /// @brief Wavelength for kinematics equations (m).
  double lambda;

  /// @brief Acceleration values to initiate track (Hz/s).
  std::vector<double> accInit;

  /// @brief Index of detections already updated.
  std::vector<bool> doNotInitiate;

  /// @brief POSIX timestamp of last update (ms).
  uint64_t timestamp;

  /// @brief Track data.
  Track track;

public:
  /// @brief Constructor.
  /// @param m Track initiation constant for M of N detections.
  /// @param n Track initiation constant for M of N detections.
  /// @param nDelete Number of missed predictions to delete a tentative track.
  /// @param cpi True CPI time for acceleration resolution(s).
  /// @param maxAccInit Maximum acceleration to initiate track (Hz/s).
  /// @param rangeRes Range resolution for kinematics equations (m).
  /// @param lambda Wavelength for kinematics equations (m).
  /// @return The object.
  Tracker(uint32_t m, uint32_t n, uint32_t nDelete, double cpi, 
    double maxAccInit, double rangeRes, double lambda);

  /// @brief Destructor.
  /// @return Void.
  ~Tracker();

  /// @brief Run through key functions of tracker.
  /// @param detection Detection data for last CPI.
  /// @param timestamp POSIX timestamp (ms).
  /// @return Pointer to track data.
  std::unique_ptr<Track> process(Detection *detection, uint64_t timestamp);

  /// @brief Update tracks by associating detections.
  /// @param detection Detection data for last CPI.
  /// @param timestamp POSIX timestamp (ms).
  /// @return Void.
  void update(Detection *detection, uint64_t timestamp);

  /// @brief Predict next bistatic position using kinematics equations.
  /// @param current Current position of track.
  /// @param acc Acceleration hypothesis of track.
  /// @param T Time elapsed from previous CPI.
  /// @return Predicted position of track.
  Detection predict(Detection current, double acc, double T);

  /// @brief Initiate new tentative tracks from detections.
  /// @param detection Detection data for last CPI.
  /// @return Void.
  void initiate(Detection *detection);
};

#endif