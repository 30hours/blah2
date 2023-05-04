#ifndef IQDATA_H
#define IQDATA_H

#include <stdint.h>
#include <deque>
#include <complex>

class IqData
{
private:
  uint32_t n;
  bool doNotPush;
  std::deque<std::complex<double>> *data;

public:
  IqData(uint32_t n);
  uint32_t get_n();
  uint32_t get_length();
  void set_doNotPush(bool doNotPush);
  bool get_doNotPush();
  std::deque<std::complex<double>> get_data();
  void push_back(std::complex<double> sample);
  std::complex<double> pop_front();
  void print();
  void clear();
};

#endif