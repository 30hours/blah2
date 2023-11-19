/// @file IqData.h
/// @class IqData
/// @brief A class to store IQ data.
/// @details Implements a FIFO queue to store IQ samples.
/// @author 30hours

#ifndef IQDATA_H
#define IQDATA_H

#include <stdint.h>
#include <deque>
#include <complex>
#include <mutex>

class IqData
{
private:
  /// @brief Maximum number of samples.
  uint32_t n;

  /// @brief True if should not push to buffer (mutex).
  std::mutex mutex_lock;

  /// @brief Pointer to IQ data.
  std::deque<std::complex<double>> *data;

public:
  /// @brief Constructor.
  /// @param n Number of samples.
  /// @return The object.
  IqData(uint32_t n);

  /// @brief Getter for maximum number of samples.
  /// @return Maximum number of samples.
  uint32_t get_n();

  /// @brief Getter for current data length.
  /// @return Number of samples currently in data.
  uint32_t get_length();

  /// @brief Locker for mutex.
  /// @return Void.
  void lock();

  /// @brief Unlocker for mutex.
  /// @return Void.
  void unlock();

  /// @brief Getter for data.
  /// @return IQ data.
  std::deque<std::complex<double>> get_data();

  /// @brief Push a sample to the queue.
  /// @param sample A single sample.
  /// @return Void.
  void push_back(std::complex<double> sample);

  /// @brief Pop the front of the queue.
  /// @return Sample from the front of the queue.
  std::complex<double> pop_front();

  /// @brief Print to stdout (debug).
  /// @return Void.
  void print();

  /// @brief Clear samples from the queue.
  /// @return Void.
  void clear();
};

#endif