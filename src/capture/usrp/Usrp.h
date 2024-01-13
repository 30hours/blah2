/// @file Usrp.h
/// @class Usrp
/// @brief A class to capture data on the Ettus Research USRP.
/// @details Uses the UHD C API to extract samples into the processing chain.
/// 
/// Should work on all USRP models EXCEPT discontinued (USRP1, USRP2).
/// Networked models require an IP address in the config file.
///
/// @author 30hours

#ifndef USRP_H
#define USRP_H

#include "data/IqData.h"

#include <stdint.h>
#include <string>

#define BUFFER_SIZE_NR 1024

class Usrp
{
private:
  /// @brief Center frequency (Hz)
  uint32_t fc;
  
  /// @brief File path.
  std::string path;
  /// @brief True if capture is enabled.
  bool capture;

public:
  /// @brief Constructor.
  /// @param fc Center frequency (Hz).
  /// @param path Path to save IQ data.
  /// @return The object.
  Usrp(uint32_t fc, std::string path);

  /// @brief Get file name from path.
  /// @return String of file name based on current time.
  std::string set_file(std::string path);

  /// @brief Call methods to start capture.
  /// @return Void.
  void start();

  /// @brief Call methods to gracefully stop capture.
  /// @return Void.
  void stop();

  /// @brief Implement capture function on RSPduo.
  /// @param buffer1 Pointer to reference buffer.
  /// @param buffer2 Pointer to surveillance buffer.
  /// @return Void.
  void process(IqData *buffer1, IqData *buffer2);

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

  /// @brief Setter for capture.
  /// @param capture True if capture is enabled.
  /// @return Void.
  void set_capture(bool capture);

  /// @brief Getter for capture.
  /// @return True if capture is true.
  bool get_capture();
};

#endif