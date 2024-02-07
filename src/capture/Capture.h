/// @file Capture.h
/// @class Capture
/// @brief A class for a generic IQ capture device.
/// @author 30hours

#ifndef CAPTURE_H
#define CAPTURE_H

#include <string>
#include <memory>
#include <ryml/ryml.hpp>
#include <ryml/ryml_std.hpp> // optional header, provided for std:: interop
#include <c4/format.hpp> // needed for the examples below

#include "data/IqData.h"
#include "capture/Source.h"

class Capture
{
private:
  /// @brief The valid capture devices.
  static const std::string VALID_TYPE[3];

  /// @brief The capture device type.
  std::string type;

  /// @brief True if IQ data to be saved.
  bool saveIq;

  /// @brief True if file replay is enabled.
  bool replay;

  /// @brief True if replay file should loop when complete.
  bool loop;

  /// @brief Absolute path of file to replay.
  std::string file;

public:

  /// @brief Sampling frequency (Hz).
  uint32_t fs;

  /// @brief Center frequency (Hz).
  uint32_t fc;

  /// @brief Absolute path to IQ save location.
  std::string path;

  /// @brief Constructor.
  /// @param type The capture device type.
  /// @param fs Sampling frequency (Hz).
  /// @param fc Center frequency (Hz).
  /// @param path Absolute path to IQ save location.
  /// @return The object.
  Capture(std::string type, uint32_t fs, uint32_t fc, std::string path);

  /// @brief Implement the capture process.
  /// @param buffer1 Buffer for reference samples.
  /// @param buffer2 Buffer for surveillance samples.
  /// @param config Yaml config for device.
  /// @param ip_capture IP address of capture API.
  /// @param port_capture Port of capture API.
  /// @return Void.
  void process(IqData *buffer1, IqData *buffer2, c4::yml::NodeRef config, 
    std::string ip_capture, uint16_t port_capture);

  std::unique_ptr<Source> factory_source(const std::string& type, 
    c4::yml::NodeRef config);

  /// @brief Set parameters to enable file replay.
  /// @param loop True if replay file should loop when complete.
  /// @param file Absolute path of file to replay.
  /// @return Void.
  void set_replay(bool loop, std::string file);

};

#endif