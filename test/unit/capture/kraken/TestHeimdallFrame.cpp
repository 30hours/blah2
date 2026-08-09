#include "capture/kraken/HeimdallFrame.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstring>

namespace
{
void write_be_u32(
  std::array<uint8_t, HeimdallFrame::FIXED_HEADER_SIZE>& data,
  std::size_t offset, uint32_t value)
{
  for (std::size_t byte = 0; byte < 4; byte++)
    data[offset + byte] = static_cast<uint8_t>(value >> (8 * (3 - byte)));
}

void append_le_float(std::vector<uint8_t>& data, float value)
{
  uint32_t bits;
  std::memcpy(&bits, &value, sizeof(bits));
  for (std::size_t byte = 0; byte < 4; byte++)
    data.push_back(static_cast<uint8_t>(bits >> (8 * byte)));
}

std::array<uint8_t, HeimdallFrame::FIXED_HEADER_SIZE> valid_header()
{
  std::array<uint8_t, HeimdallFrame::FIXED_HEADER_SIZE> bytes{};
  write_be_u32(bytes, 0, HeimdallFrame::MAGIC);
  write_be_u32(bytes, 4, 5);
  write_be_u32(bytes, 8, 2);
  write_be_u32(bytes, 12, HeimdallFrame::CALIBRATED);
  return bytes;
}
}

TEST_CASE("Heimdall V2 five-channel frames decode")
{
  const auto bytes = valid_header();
  auto header = HeimdallFrame::decode_header(bytes);
  REQUIRE(header.numChannels == 5);
  REQUIRE(header.samplesPerChannel == 2);
  REQUIRE(HeimdallFrame::is_synchronized_data(header));
  REQUIRE(HeimdallFrame::metadata_size(header) == 40);
  REQUIRE(HeimdallFrame::payload_size(header) == 20);

  std::vector<uint8_t> metadata;
  for (uint32_t channel = 0; channel < 5; channel++)
  {
    append_le_float(metadata, 527000000.0F);
    append_le_float(metadata, 15.0F + channel);
  }
  HeimdallFrame::decode_metadata(header, metadata);
  REQUIRE(header.frequencies[4] == 527000000.0F);
  REQUIRE(header.gains[4] == 19.0F);

  std::vector<uint8_t> payload;
  for (uint32_t channel = 0; channel < 5; channel++)
  {
    payload.push_back(0);
    payload.push_back(64);
    payload.push_back(255);
    payload.push_back(192);
  }
  const auto channels = HeimdallFrame::decode_payload(header, payload);
  REQUIRE(channels.size() == 5);
  REQUIRE(channels[4][0].real() == -1.0F);
  REQUIRE(channels[4][1].real() == 1.0F);
  REQUIRE(channels[4][0].imag() == Catch::Approx(-64.0 / 127.5));
  REQUIRE(channels[4][1].imag() == Catch::Approx(64.0 / 127.5));
}

TEST_CASE("Heimdall V2 accepts supported channel counts")
{
  for (const uint32_t channelCount : {2U, 3U, 4U, 5U, 6U, 7U, 8U})
  {
    auto bytes = valid_header();
    write_be_u32(bytes, 4, channelCount);
    auto header = HeimdallFrame::decode_header(bytes);
    REQUIRE(header.numChannels == channelCount);
    REQUIRE(HeimdallFrame::metadata_size(header) == channelCount * 8);
    REQUIRE(HeimdallFrame::payload_size(header) == channelCount * 4);
  }
}

TEST_CASE("Heimdall V2 rejects calibration and retune frames")
{
  auto bytes = valid_header();
  write_be_u32(bytes, 12, 5);
  REQUIRE_FALSE(HeimdallFrame::is_synchronized_data(
    HeimdallFrame::decode_header(bytes)));

  bytes = valid_header();
  write_be_u32(bytes, 16, 1);
  REQUIRE_FALSE(HeimdallFrame::is_synchronized_data(
    HeimdallFrame::decode_header(bytes)));

  bytes = valid_header();
  write_be_u32(bytes, 28, 1);
  REQUIRE_FALSE(HeimdallFrame::is_synchronized_data(
    HeimdallFrame::decode_header(bytes)));

  bytes = valid_header();
  write_be_u32(bytes, 12, 0x100U | HeimdallFrame::CALIBRATED);
  REQUIRE(HeimdallFrame::is_synchronized_data(
    HeimdallFrame::decode_header(bytes)));
}

TEST_CASE("Heimdall V2 validates frame dimensions")
{
  auto bytes = valid_header();
  write_be_u32(bytes, 4, 9);
  REQUIRE_THROWS(HeimdallFrame::decode_header(bytes));

  bytes = valid_header();
  write_be_u32(bytes, 8, 0);
  REQUIRE_THROWS(HeimdallFrame::decode_header(bytes));

  auto header = HeimdallFrame::decode_header(valid_header());
  REQUIRE_THROWS(HeimdallFrame::decode_metadata(header, {}));
  REQUIRE_THROWS(HeimdallFrame::decode_payload(header, {}));
}
