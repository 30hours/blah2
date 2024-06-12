/// @file Kraken.h
/// @class Kraken
/// @brief A class to capture data on the Kraken SDR.
/// @details Uses a custom librtlsdr API to extract samples.
/// Uses 2 channels of the Kraken to capture IQ data.
/// The noise source phase synchronisation is not required for 2 channel operation.
/// Future work is to replicate the Heimdall DAQ phase syncronisation.
/// This will enable a surveillance array of up to 4 antenna elements.
/// Requires a custom librtlsdr which includes method rtlsdr_set_dithering().
/// The original steve-m/librtlsdr does not include this method.
/// This is included in librtlsdr/librtlsdr or krakenrf/librtlsdr.
/// Also works using 2 RTL-SDRs which have been clock synchronised.
/// @author 30hours, Michael Brock, sdn-ninja
/// @todo Add support for multiple surveillance channels.
/// @todo Replay support.

#ifndef KRAKEN_H
#define KRAKEN_H

#include "capture/Source.h"
#include "data/IqData.h"

#include <stdint.h>
#include <string>
#include <vector>
#include <rtl-sdr.h>

class Kraken : public Source
{
private:

  /// @brief Individual RTL-SDR devices.
  rtlsdr_dev_t* devs[5];

  /// @brief Device indices for Kraken.
  std::vector<int> channelIndex;

  /// @brief Gain for each channel.
  std::vector<int> gain;

  /// @brief Check status of API returns.
  /// @param status Return code of API call.
  /// @param message Message if API call error.
  /// @return Void.
  void check_status(int status, std::string message);

  /// @brief Callback function when buffer is filled.
  /// @param buf Pointer to buffer of IQ data.
  /// @param len Length of buffer.
  /// @param ctx Context data for callback.
  /// @return Void.
  static void callback(unsigned char *buf, uint32_t len, void *ctx);

public:

  /// @brief Constructor.
  /// @param fc Center frequency (Hz).
  /// @param path Path to save IQ data.
  /// @return The object.
  Kraken(std::string type, uint32_t fc, uint32_t fs, std::string path, 
    bool *saveIq, std::vector<double> gain);

  /// @brief Implement capture function on KrakenSDR.
  /// @param buffer Pointers to buffers for each channel.
  /// @return Void.
  void process(IqData *buffer1, IqData *buffer2);

  /// @brief Call methods to start capture.
  /// @return Void.
  void start();

  /// @brief Call methods to gracefully stop capture.
  /// @return Void.
  void stop();

  /// @brief Implement replay function on the Kraken.
  /// @param buffers Pointers to buffers for each channel.
  /// @param file Path to file to replay data from.
  /// @param loop True if samples should loop at EOF.
  /// @return Void.
  void replay(IqData *buffer1, IqData *buffer2, std::string file, bool loop);

};

#endif
