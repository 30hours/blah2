/// @file Timing.h
/// @class Timing
/// @brief A class to store timing statistics.
/// @author 30hours

#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>
#include <vector>
#include <string>

class Timing
{
private:
  /// @brief Start time (POSIX ms).
  uint64_t tStart;

  /// @brief Current time (POSIX ms).
  uint64_t tNow;

  /// @brief Number of CPI's.
  uint64_t n;

  /// @brief Time since first CPI (ms).
  uint64_t uptime;

  /// @brief Time differences (ms).
  std::vector<double> time;

  /// @brief Names of time differences.
  std::vector<std::string> name;

public:
  /// @brief Constructor.
  /// @param tStart Start time (POSIX ms).
  /// @return The object.
  Timing(uint64_t tStart);

  /// @brief Update the time differences and names.
  /// @param tNow Current time (POSIX ms).
  /// @param time Vector of time differences (ms).
  /// @param name Vector of time difference names.
  /// @return Void.
  void update(uint64_t tNow, std::vector<double> time, std::vector<std::string> name);

  /// @brief Generate JSON of the map and metadata.
  /// @return JSON string.
  std::string to_json();

  /// @brief Append the map to a save file.
  /// @param json JSON string of map and metadata.
  /// @param path Path of file to save.
  /// @return True is save is successful.
  bool save(std::string json, std::string path);
};

#endif
