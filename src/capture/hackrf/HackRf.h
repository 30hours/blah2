/// @file HackRf.h
/// @class HackRf
/// @brief A class to capture data on the HackRF.
/// @author sdn-ninja
/// @author 30hours
/// @todo Replay functionality.

#ifndef HACKRF_H
#define HACKRF_H

#include "capture/Source.h"
#include "data/IqData.h"

#include <stdint.h>
#include <string>
#include <vector>
#include <libhackrf/hackrf.h>

class HackRf : public Source
{
private:

  /// @brief Vector of serial numbers.
  /// @details Serial as given by hackrf_info.
  std::vector<std::string> serial;

  /// @brief RX LNA (IF) gain, 0-40dB, 8dB steps.
  std::vector<uint32_t> gainLna;

  /// @brief RX VGA (baseband) gain, 0-62dB, 2dB steps.
  std::vector<uint32_t> gainVga;

  /// @brief Enable extra amplifier U13 on receive.
  std::vector<bool> ampEnable;

  /// @brief Check status of HackRF API returns.
  /// @param status Return code of API call.
  /// @param message Message if API call error.
  void check_status(uint8_t status, std::string message);

protected:
  /// @brief Array of pointers to HackRF devices.
  hackrf_device* dev[2];

  /// @brief Callback function for HackRF samples.
  /// @param transfer HackRF transfer object.
  /// @return Void.
  static int rx_callback(hackrf_transfer* transfer);

public:

  /// @brief Constructor.
  /// @param fc Center frequency (Hz).
  /// @param path Path to save IQ data.
  /// @return The object.
  HackRf(std::string type, uint32_t fc, uint32_t fs, std::string path, 
    bool *saveIq, std::vector<std::string> serial, 
    std::vector<uint32_t> gainLna, std::vector<uint32_t> gainVga, 
    std::vector<bool> ampEnable);

  /// @brief Implement capture function on HackRF.
  /// @param buffer1 Pointer to reference buffer.
  /// @param buffer2 Pointer to surveillance buffer.
  /// @return Void.
  void process(IqData *buffer1, IqData *buffer2);

  /// @brief Call methods to start capture.
  /// @return Void.
  void start();

  /// @brief Call methods to gracefully stop capture.
  /// @return Void.
  void stop();

  /// @brief Implement replay function on HackRF.
  /// @param buffer1 Pointer to reference buffer.
  /// @param buffer2 Pointer to surveillance buffer.
  /// @param file Path to file to replay data from.
  /// @param loop True if samples should loop at EOF.
  /// @return Void.
  void replay(IqData *buffer1, IqData *buffer2, std::string file, bool loop);

};

#endif
