/// @file Track.h
/// @class Track
/// @brief A class to store track data.
/// @details The ID is 4 digit hexadecimal with 16^4 = 65536 combinations.
/// @details The state can be TENTATIVE, ACTIVE or COASTING.
/// @details Associated detections use null detections when no updates.
/// @author 30hours

#ifndef TRACK_H
#define TRACK_H

#include <Detection.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <memory>

class Track
{
private:
  /// @brief Track ID (4 digit alpha-numeric).
  //std::unique_ptr<std::vector<std::string>> id;
  std::vector<std::string> id;

  /// @brief Current state (see VALID_STATE).
  std::vector<std::string> state;

  /// @brief Curent track position.
  std::vector<Detection> current;

  /// @brief Current acceleration (Hz/s).
  std::vector<double> acceleration;

  /// @brief Associated detections in track.
  std::vector<std::vector<Detection>> associated;

  /// @brief Number of updates the track has been tentative/coasting.
  /// @details Forms criteria for track deletion.
  std::vector<uint64_t> nInactive;

  /// @brief Next valid track index.
  uint64_t iNext;

  /// @brief Maximum integer index to wrap around.
  static const uint64_t MAX_INDEX;

  /// @brief String for state ACTIVE.
  static const std::string STATE_ACTIVE;

  /// @brief String for state TENTATIVE.
  static const std::string STATE_TENTATIVE;

  /// @brief String for state COASTING.
  static const std::string STATE_COASTING;

public:
  /// @brief Constructor.
  /// @return The object.
  Track();

  /// @brief Destructor.
  /// @return Void.
  ~Track();

  /// @brief Convert an unsigned int to hexadecimal.
  /// @details Max number is 16^4 = 65536 before wrap around.
  /// @param number Number to convert to hexadecimal.
  /// @return hex Hexadecimal number.
  std::string uint2hex(uint64_t number);

  /// @brief Set the state of a track.
  /// @param index Index of track to change.
  /// @param state Updated state.
  /// @return Void.
  void set_state(uint64_t index, std::string state);

  /// @brief Set the current track position.
  /// @details Use to update smoothed current position.
  /// @param index Index of track to change.
  /// @param smoothed Updated state.
  /// @return Void.
  void set_current(uint64_t index, Detection smoothed);

  /// @brief Set the current acceleration.
  /// @param index Index of track to change.
  /// @param acceleration Updated acceleration.
  /// @return Void.
  void set_acceleration(uint64_t index, double acceleration);

  /// @brief Set the current inactivity.
  /// @param index Index of track to change.
  /// @param n Updated inactivity index.
  /// @return Void.
  void set_nInactive(uint64_t index, uint64_t n);

  /// @brief Get number of active tracks.
  /// @return Number of active tracks.
  uint64_t get_nActive();

  /// @brief Get number of tentative tracks.
  /// @return Number of tentative tracks.
  uint64_t get_nTentative();

  /// @brief Get number of total tracks.
  /// @return Number of total tracks.
  uint64_t get_n();

  /// @brief Get current track position for track index.
  /// @return Current detection.
  Detection get_current(uint64_t index);

  /// @brief Get current acceleration for track index.
  /// @return Current acceleration (Hz/s).
  double get_acceleration(uint64_t index);

  /// @brief Get current state for track index.
  /// @return Current state.
  std::string get_state(uint64_t index);

  /// @brief Get number of updates track has been tentative/coasting.
  /// @return Number of updates track has been tentative/coasting.
  uint64_t get_nInactive(uint64_t index);

  /// @brief Update an associated detection.
  /// @param index Index of track to change.
  /// @param update New associated detection.
  /// @return Void.
  void update(uint64_t index, Detection update);

  /// @brief Add track to the track set.
  /// @param initial Initial Detection.
  /// @details ID is incremented automatically. 
  /// @details Initial state is always TENTATIVE.
  /// @return Index of last track.
  uint64_t add(Detection initial);

  /// @brief Remove track based on index.
  /// @param index Index of track to remove.
  /// @return Void.
  void remove(uint64_t index);

  /// @brief Generate JSON of the map and metadata.
  /// @param timestamp Current time (POSIX ms).
  /// @return JSON string.
  std::string to_json(uint64_t timestamp);

  /// @brief Append the map to a save file.
  /// @param json JSON string of map and metadata.
  /// @param path Path of file to save.
  /// @return True is save is successful.
  bool save(std::string json, std::string path);
};

#endif
