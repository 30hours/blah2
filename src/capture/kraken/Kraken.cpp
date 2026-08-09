#include "Kraken.h"

#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <optional>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace
{
class ConnectionError : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};
}

Kraken::Kraken(std::string type, uint32_t fc, uint32_t fs,
  std::string path, bool *saveIq, std::size_t channelCount,
  std::string heimdallHost, uint16_t heimdallPort)
  : Source(type, fc, fs, path, saveIq), channelCount(channelCount),
    heimdallHost(heimdallHost), heimdallPort(heimdallPort)
{
  if (channelCount == 0 || channelCount > MAX_CHANNELS)
    throw std::invalid_argument("Kraken requires between one and eight channels");
  if (fs != SUITE_SAMPLE_RATE)
    throw std::invalid_argument(
      "KrakenSDR Suite V2 requires capture.fs to be 2400000");
}

void Kraken::start()
{
  running = true;
}

void Kraken::stop()
{
  running = false;
  if (socketFd >= 0)
  {
    shutdown(socketFd, SHUT_RDWR);
    close(socketFd);
    socketFd = -1;
  }
}

void Kraken::process(IqData *buffer1, IqData *buffer2)
{
  process(std::vector<IqData *>{buffer1, buffer2});
}

void Kraken::process(const std::vector<IqData *>& buffers)
{
  if (buffers.empty() || buffers.size() > MAX_CHANNELS ||
      buffers.size() != channelCount)
    throw std::invalid_argument("Kraken buffers must match configured channels");
  process_heimdall(buffers);
}

void Kraken::process_heimdall(const std::vector<IqData *>& buffers)
{
  while (running)
  {
    try
    {
      stream_heimdall(buffers);
    }
    catch (const ConnectionError& error)
    {
      if (socketFd >= 0)
      {
        close(socketFd);
        socketFd = -1;
      }
      if (!running) return;
      clear_buffers(buffers);
      std::cerr << "[Kraken] " << error.what()
        << "; retrying in 1 second" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

void Kraken::stream_heimdall(const std::vector<IqData *>& buffers)
{
  connect_heimdall();
  std::optional<uint32_t> frequencyChangeCounter;
  bool resetPending = false;
  while (running)
  {
    const auto headerBytes = receive_header();
    auto header = HeimdallFrame::decode_header(headerBytes);
    std::vector<uint8_t> metadata(HeimdallFrame::metadata_size(header));
    receive_exact(metadata.data(), metadata.size());
    HeimdallFrame::decode_metadata(header, metadata);
    std::vector<uint8_t> payload(HeimdallFrame::payload_size(header));
    receive_exact(payload.data(), payload.size());

    if (header.numChannels != buffers.size())
      throw std::runtime_error(
        "Heimdall V2 channel count does not match BLAH2 configuration");
    for (float frequency : header.frequencies)
      if (std::abs(static_cast<double>(frequency) - fc) > 256.0)
        throw std::runtime_error(
          "Heimdall V2 frequency does not match BLAH2 configuration");

    if (frequencyChangeCounter &&
        *frequencyChangeCounter != header.frequencyChangeCounter)
      resetPending = true;
    frequencyChangeCounter = header.frequencyChangeCounter;
    if (!HeimdallFrame::is_synchronized_data(header))
    {
      resetPending = true;
      continue;
    }
    if (resetPending)
    {
      clear_buffers(buffers);
      resetPending = false;
    }

    const auto channels = HeimdallFrame::decode_payload(header, payload);
    for (auto *buffer : buffers) buffer->lock();
    for (std::size_t channel = 0; channel < channels.size(); channel++)
      buffers[channel]->append_unlocked(channels[channel]);
    for (auto it = buffers.rbegin(); it != buffers.rend(); ++it)
      (*it)->unlock();

    if (*saveIq && saveIqFile.is_open())
    {
      saveIqFile.write(reinterpret_cast<const char *>(headerBytes.data()),
        static_cast<std::streamsize>(headerBytes.size()));
      saveIqFile.write(reinterpret_cast<const char *>(metadata.data()),
        static_cast<std::streamsize>(metadata.size()));
      saveIqFile.write(reinterpret_cast<const char *>(payload.data()),
        static_cast<std::streamsize>(payload.size()));
    }
  }
}

void Kraken::connect_heimdall()
{
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo *addresses = nullptr;
  const std::string port = std::to_string(heimdallPort);
  if (getaddrinfo(heimdallHost.c_str(), port.c_str(), &hints, &addresses) != 0)
    throw ConnectionError("Unable to resolve Heimdall V2 host");
  std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> guard(addresses, freeaddrinfo);
  for (auto *address = addresses; address; address = address->ai_next)
  {
    socketFd = socket(address->ai_family, address->ai_socktype,
      address->ai_protocol);
    if (socketFd >= 0 &&
        connect(socketFd, address->ai_addr, address->ai_addrlen) == 0)
    {
      std::cout << "[Kraken] Connected to KrakenSDR Suite V2 Heimdall at "
        << heimdallHost << ":" << heimdallPort << std::endl;
      return;
    }
    if (socketFd >= 0) close(socketFd);
    socketFd = -1;
  }
  throw ConnectionError("Unable to connect to Heimdall V2");
}

void Kraken::receive_exact(void *data, std::size_t length)
{
  auto *bytes = static_cast<uint8_t *>(data);
  std::size_t received = 0;
  while (received < length)
  {
    const ssize_t count = recv(socketFd, bytes + received, length - received, 0);
    if (count == 0) throw ConnectionError("Heimdall V2 closed connection");
    if (count < 0)
    {
      if (errno == EINTR) continue;
      throw ConnectionError(std::strerror(errno));
    }
    received += static_cast<std::size_t>(count);
  }
}

std::array<uint8_t, HeimdallFrame::FIXED_HEADER_SIZE> Kraken::receive_header()
{
  constexpr std::array<uint8_t, 4> magic{'M', 'C', 'H', 'Q'};
  std::array<uint8_t, HeimdallFrame::FIXED_HEADER_SIZE> header{};
  std::size_t matched = 0;
  while (matched < magic.size())
  {
    uint8_t byte;
    receive_exact(&byte, 1);
    if (byte == magic[matched])
      header[matched++] = byte;
    else
      matched = byte == magic[0] ? 1 : 0;
  }
  receive_exact(header.data() + magic.size(), header.size() - magic.size());
  return header;
}

void Kraken::clear_buffers(const std::vector<IqData *>& buffers)
{
  for (auto *buffer : buffers) buffer->lock();
  for (auto *buffer : buffers) buffer->clear();
  for (auto it = buffers.rbegin(); it != buffers.rend(); ++it)
    (*it)->unlock();
}

void Kraken::replay(IqData *, IqData *, std::string, bool)
{
  throw std::runtime_error("Kraken replay is not implemented");
}

void Kraken::replay(const std::vector<IqData *>&, std::string, bool)
{
  throw std::runtime_error("Kraken replay is not implemented");
}
