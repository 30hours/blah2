/// @file Map.h
/// @class Map
/// @brief A class to store an ambiguity map.
/// @details References:
/// <a href="https://stackoverflow.com/questions/39110263/append-to-an-existing-array-of-json-objects-on-file-using-rapidjson">
/// Append to an existing array using rapidjson.</a>
/// @author 30hours

#ifndef MAP_H
#define MAP_H

#include <stdint.h>
#include <vector>
#include <deque>
#include <complex>

template <typename T>

class Map
{
private:
  /// @brief Number of rows.
  uint32_t nRows;

  /// @brief Number of columns.
  uint32_t nCols;

public:
  /// @brief Map data to store.
  std::vector<std::vector<T>> data;

  /// @brief Delay units of map data (bins).
  std::deque<int> delay;

  /// @brief Doppler units of map data (Hz).
  std::deque<double> doppler;

  /// @brief Noise power of map (dB).
  double noisePower;

  /// @brief Dynamic range of map (dB).
  double maxPower;

  /// @brief Constructor.
  /// @param nRows Number of rows.
  /// @param nCols Number of columns.
  /// @return The object.
  Map(uint32_t nRows, uint32_t nCols);

  /// @brief Update a row in the 2D map.
  /// @param i Index of row to update.
  /// @param row Data to update.
  /// @return Void.
  void set_row(uint32_t i, std::vector<T> row);

  /// @brief Update a column in the 2D map.
  /// @param i Index of column to update.
  /// @param col Data to update.
  /// @return Void.
  void set_col(uint32_t i, std::vector<T> col);

  /// @brief Create map metrics (noise power, dynamic range).
  /// @return Void.
  void set_metrics();

  /// @brief Get the number of rows in the map.
  /// @return Number of rows.
  uint32_t get_nRows();

  /// @brief Get the number of columns in the map.
  /// @return Number of columns.
  uint32_t get_nCols();

  /// @brief Get a row from the 2D map.
  /// @param row Index of row to get.
  /// @return Vector of data.
  std::vector<T> get_row(uint32_t row);

  /// @brief Get a column from the 2D map.
  /// @param col Index of column to get.
  /// @return Vector of data.
  std::vector<T> get_col(uint32_t col);

  /// @brief Get a copy of the map in dB units.
  /// @return Pointer to dB map.
  Map<double> *get_map_db();

  /// @brief Print the map to stdout (for debugging).
  /// @return Void.
  void print();

  /// @brief Generate JSON of the map and metadata.
  /// @return JSON string.
  std::string to_json();

  /// @brief Update JSON to convert delay bins to km.
  /// @param json Input JSON string with delay field.
  /// @param fs Sampling frequency (Hz).
  /// @return JSON string.
  std::string delay_bin_to_km(std::string json, uint32_t fs);

  /// @brief Append the map to a save file.
  /// @param json JSON string of map and metadata.
  /// @param path Path of file to save.
  /// @return True is save is successful.
  bool save(std::string json, std::string path);
};

#endif
