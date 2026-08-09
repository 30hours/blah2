#include "HeimdallFrame.h"

#include <cstring>
#include <limits>
#include <stdexcept>

namespace
{
uint32_t read_be_u32(const uint8_t *bytes)
{
  return (static_cast<uint32_t>(bytes[0]) << 24) |
    (static_cast<uint32_t>(bytes[1]) << 16) |
    (static_cast<uint32_t>(bytes[2]) << 8) |
    static_cast<uint32_t>(bytes[3]);
}

uint32_t read_le_u32(const uint8_t *bytes)
{
  return static_cast<uint32_t>(bytes[0]) |
    (static_cast<uint32_t>(bytes[1]) << 8) |
    (static_cast<uint32_t>(bytes[2]) << 16) |
    (static_cast<uint32_t>(bytes[3]) << 24);
}

float read_le_float(const uint8_t *bytes)
{
  const uint32_t bits = read_le_u32(bytes);
  float value;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}
}

HeimdallFrame::Header HeimdallFrame::decode_header(
  const std::array<uint8_t, FIXED_HEADER_SIZE>& bytes)
{
  if (read_be_u32(bytes.data()) != MAGIC)
    throw std::runtime_error("Invalid Heimdall V2 frame magic");

  Header header;
  header.numChannels = read_be_u32(bytes.data() + 4);
  header.samplesPerChannel = read_be_u32(bytes.data() + 8);
  header.phaseState = read_be_u32(bytes.data() + 12);
  header.noiseSource = read_be_u32(bytes.data() + 16);
  header.frequencyChangeCounter = read_be_u32(bytes.data() + 20);
  header.currentGroupIndex = read_be_u32(bytes.data() + 24);
  header.retuningInProgress = read_be_u32(bytes.data() + 28);

  if (header.numChannels == 0 || header.numChannels > 8)
    throw std::runtime_error("Invalid Heimdall V2 channel count");
  if (header.samplesPerChannel == 0 || header.samplesPerChannel > 65536)
    throw std::runtime_error("Invalid Heimdall V2 sample count");
  return header;
}

std::size_t HeimdallFrame::metadata_size(const Header& header)
{
  return static_cast<std::size_t>(header.numChannels) * 2 * sizeof(float);
}

void HeimdallFrame::decode_metadata(
  Header& header, const std::vector<uint8_t>& bytes)
{
  if (bytes.size() != metadata_size(header))
    throw std::runtime_error("Incomplete Heimdall V2 channel metadata");
  header.frequencies.resize(header.numChannels);
  header.gains.resize(header.numChannels);
  for (uint32_t channel = 0; channel < header.numChannels; channel++)
  {
    const std::size_t offset = static_cast<std::size_t>(channel) * 8;
    header.frequencies[channel] = read_le_float(bytes.data() + offset);
    header.gains[channel] = read_le_float(bytes.data() + offset + 4);
  }
}

std::size_t HeimdallFrame::payload_size(const Header& header)
{
  constexpr std::size_t bytesPerComplex = 2;
  if (header.samplesPerChannel > std::numeric_limits<std::size_t>::max() /
      header.numChannels / bytesPerComplex)
    throw std::overflow_error("Heimdall V2 payload size overflow");
  return static_cast<std::size_t>(header.samplesPerChannel) *
    header.numChannels * bytesPerComplex;
}

std::vector<std::vector<std::complex<float>>> HeimdallFrame::decode_payload(
  const Header& header, const std::vector<uint8_t>& bytes)
{
  if (bytes.size() != payload_size(header))
    throw std::runtime_error("Incomplete Heimdall V2 payload");

  std::vector<std::vector<std::complex<float>>> channels(
    header.numChannels,
    std::vector<std::complex<float>>(header.samplesPerChannel));
  for (uint32_t channel = 0; channel < header.numChannels; channel++)
  {
    const std::size_t start = static_cast<std::size_t>(channel) *
      header.samplesPerChannel * 2;
    double meanI = 0;
    double meanQ = 0;
    for (uint32_t sample = 0; sample < header.samplesPerChannel; sample++)
    {
      meanI += bytes[start + sample * 2];
      meanQ += bytes[start + sample * 2 + 1];
    }
    meanI /= header.samplesPerChannel;
    meanQ /= header.samplesPerChannel;
    for (uint32_t sample = 0; sample < header.samplesPerChannel; sample++)
    {
      channels[channel][sample] = {
        static_cast<float>((bytes[start + sample * 2] - meanI) / 127.5),
        static_cast<float>((bytes[start + sample * 2 + 1] - meanQ) / 127.5)};
    }
  }
  return channels;
}

bool HeimdallFrame::is_synchronized_data(const Header& header)
{
  return (header.phaseState & 0xffU) == CALIBRATED &&
    header.noiseSource == 0 && header.retuningInProgress == 0;
}
