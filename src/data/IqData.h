/// @file IqData.h
/// @class IqData
/// @brief A class to store IQ data.
/// @details Implements a FIFO queue to store IQ samples.
/// @author 30hours

#ifndef IQDATA_H
#define IQDATA_H

#include <stdint.h>
#include <deque>
#include <vector>
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

  /// @brief Minimum value.
  double min;

  /// @brief Maximum value.
  double max;

  /// @brief Mean value.
  double mean;

  /// @brief Spectrum vector.
  std::vector<std::complex<double>> spectrum;

  /// @brief Frequency vector (Hz).
  std::vector<double> frequency;

public:
  /// @brief Constructor.
  /// @param n Number of samples.
  /// @return The object.
  IqData(uint32_t n);

  /// @brief Destructor.
  ~IqData();

  IqData(const IqData&) = delete;
  IqData& operator=(const IqData&) = delete;

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

  /// @brief Read-only access without copying. Caller must prevent mutation.
  const std::deque<std::complex<double>>& view_data() const;

  /// @brief Remove and return a block from the front of the queue.
  std::deque<std::complex<double>> drain_front(uint32_t count);

  /// @brief Replace all samples with an existing block.
  void replace(std::deque<std::complex<double>>&& samples);

  /// @brief Push a sample to the queue.
  /// @param sample A single sample.
  /// @return Void.
  void push_back(std::complex<double> sample);

  /// @brief Append a coherent float32 block while retaining the newest n.
  /// @warning Caller must hold this object's lock.
  void append_unlocked(const std::vector<std::complex<float>>& samples);

  /// @brief Pop the front of the queue.
  /// @return Sample from the front of the queue.
  std::complex<double> pop_front();

  /// @brief Print to stdout (debug).
  /// @return Void.
  void print();

  /// @brief Clear samples from the queue.
  /// @return Void.
  void clear();

  /// @brief Update the time differences and names.
  /// @param spectrum Spectrum vector.
  /// @return Void.
  void update_spectrum(std::vector<std::complex<double>> spectrum);

  /// @brief Update the time differences and names.
  /// @param frequency Frequency vector.
  /// @return Void.
  void update_frequency(std::vector<double> frequency);

  /// @brief Generate JSON of the signal and metadata.
  /// @param timestamp Current time (POSIX ms).
  /// @return JSON string.
  std::string to_json(uint64_t timestamp);
};

#endif
