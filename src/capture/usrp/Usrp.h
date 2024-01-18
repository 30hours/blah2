/// @file Usrp.h
/// @class Usrp
/// @brief A class to capture data on the Ettus Research USRP.
/// @details Uses the UHD C API to extract samples into the processing chain.
/// 
/// Should work on all USRP models EXCEPT discontinued (USRP1, USRP2).
/// Networked models require an IP address in the config file.
/// Requires a USB 3.0 cable for higher data rates.
///
/// @author 30hours
/// @todo Add replay to Usrp.
/// @todo Add IQ data saving to Usrp.
/// @todo Fix single overflow per CPI.
/// @todo Fix occasional timeout ERROR_CODE_TIMEOUT.

#ifndef USRP_H
#define USRP_H

#include "capture/Source.h"
#include "data/IqData.h"

#include <stdint.h>
#include <string>

class Usrp : public Source
{
private:

  /// @brief Address of USRP device.
  /// @details "localhost" if USB, else IP address.
  std::string address;

  /// @brief Subdevice string for USRP.
  /// @details See docs.
  std::string subdev;

  /// @brief Antenna string for each channel.
  std::vector<std::string> antenna;

  /// @brief USRP gain for each channel.
  std::vector<double> gain;

public:
  /// @brief Constructor.
  /// @param fc Center frequency (Hz).
  /// @param path Path to save IQ data.
  /// @return The object.
  Usrp(std::string type, uint32_t fc, uint32_t fs, std::string path, 
    bool *saveIq, std::string address, std::string subdev, 
    std::vector<std::string> antenna, std::vector<double> gain);

  /// @brief Get file name from path.
  /// @return String of file name based on current time.
  std::string set_file(std::string path);

  /// @brief Implement capture function on USRP.
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

  /// @brief Implement replay function on RSPduo.
  /// @param buffer1 Pointer to reference buffer.
  /// @param buffer2 Pointer to surveillance buffer.
  /// @param file Path to file to replay data from.
  /// @param loop True if samples should loop at EOF.
  /// @return Void.
  void replay(IqData *buffer1, IqData *buffer2, std::string file, bool loop);

  /// @brief Open a new file to record IQ.
  /// @return Void.
  void open_file();

  /// @brief Close IQ file gracefully.
  /// @return Void.
  void close_file();
};

#endif