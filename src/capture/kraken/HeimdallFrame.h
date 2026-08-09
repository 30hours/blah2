#ifndef HEIMDALL_FRAME_H
#define HEIMDALL_FRAME_H

#include <array>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

/// Decoder for the KrakenSDR Suite V2 Heimdall data stream.
class HeimdallFrame
{
public:
  static constexpr std::size_t FIXED_HEADER_SIZE = 32;
  static constexpr uint32_t MAGIC = 0x4d434851; // "MCHQ"
  static constexpr uint32_t CALIBRATED = 4;

  struct Header
  {
    uint32_t numChannels{};
    uint32_t samplesPerChannel{};
    uint32_t phaseState{};
    uint32_t noiseSource{};
    uint32_t frequencyChangeCounter{};
    uint32_t currentGroupIndex{};
    uint32_t retuningInProgress{};
    std::vector<float> frequencies;
    std::vector<float> gains;
  };

  static Header decode_header(
    const std::array<uint8_t, FIXED_HEADER_SIZE>& bytes);
  static std::size_t metadata_size(const Header& header);
  static void decode_metadata(Header& header, const std::vector<uint8_t>& bytes);
  static std::size_t payload_size(const Header& header);
  static std::vector<std::vector<std::complex<float>>> decode_payload(
    const Header& header, const std::vector<uint8_t>& bytes);
  static bool is_synchronized_data(const Header& header);
};

#endif
