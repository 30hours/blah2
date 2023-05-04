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
  uint32_t nRows;
  uint32_t nCols;

public:
  std::vector<std::vector<T>> data;
  std::deque<int> delay;
  std::deque<double> doppler;
  double noisePower;
  double maxPower;
  Map(uint32_t nRows, uint32_t nCols);
  void set_row(uint32_t i, std::vector<T> row);
  void set_col(uint32_t i, std::vector<T> col);
  void set_metrics();
  uint32_t get_nRows();
  uint32_t get_nCols();
  std::vector<T> get_row(uint32_t row);
  std::vector<T> get_col(uint32_t col);
  Map<double> *get_map_db();
  void print();
  std::string to_json();
  bool save(std::string json, std::string path);
};

#endif