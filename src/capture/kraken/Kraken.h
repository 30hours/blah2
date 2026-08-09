/// @file Kraken.h
/// @brief Coherent KrakenSDR capture through KrakenSDR Suite V2 Heimdall.

#ifndef KRAKEN_H
#define KRAKEN_H

#include "capture/Source.h"
#include "data/IqData.h"
#include "HeimdallFrame.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

class Kraken : public Source
{
private:
  static constexpr std::size_t MAX_CHANNELS = 8;
  static constexpr uint32_t SUITE_SAMPLE_RATE = 2400000;
  std::size_t channelCount;
  std::string heimdallHost;
  uint16_t heimdallPort;
  std::atomic<bool> running{false};
  int socketFd{-1};

  void process_heimdall(const std::vector<IqData *>& buffers);
  void stream_heimdall(const std::vector<IqData *>& buffers);
  void connect_heimdall();
  void receive_exact(void *data, std::size_t length);
  std::array<uint8_t, HeimdallFrame::FIXED_HEADER_SIZE> receive_header();
  static void clear_buffers(const std::vector<IqData *>& buffers);

public:
  Kraken(std::string type, uint32_t fc, uint32_t fs, std::string path,
    bool *saveIq, std::size_t channelCount, std::string heimdallHost,
    uint16_t heimdallPort);
  void process(IqData *buffer1, IqData *buffer2) override;
  void process(const std::vector<IqData *>& buffers) override;
  void start() override;
  void stop() override;
  void replay(IqData *buffer1, IqData *buffer2,
    std::string file, bool loop) override;
  void replay(const std::vector<IqData *>& buffers,
    std::string file, bool loop) override;
};

#endif
