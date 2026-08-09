#include "Source.h"

#include <iostream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

Source::Source()
{
}

// constructor
Source::Source(std::string _type, uint32_t _fc, uint32_t _fs, 
    std::string _path, bool *_saveIq)
{
  type = _type;
  fc = _fc;
  fs = _fs;
  path = _path;
  saveIq = _saveIq;
}

void Source::process(const std::vector<IqData *>& buffers)
{
  if (buffers.size() != 2)
  {
    throw std::invalid_argument("Two-channel source requires two buffers");
  }
  process(buffers[0], buffers[1]);
}

void Source::replay(const std::vector<IqData *>& buffers,
  std::string file, bool loop)
{
  if (buffers.size() != 2)
  {
    throw std::invalid_argument("Two-channel replay requires two buffers");
  }
  replay(buffers[0], buffers[1], file, loop);
}

std::string Source::open_file()
{
  // get string of timestamp in YYYYmmdd-HHMMSS
  auto currentTime = std::chrono::system_clock::to_time_t(
    std::chrono::system_clock::now());
  std::tm* timeInfo = std::localtime(&currentTime);
  std::ostringstream oss;
  oss << std::put_time(timeInfo, "%Y%m%d-%H%M%S");
  std::string timestamp = oss.str();

  // create file path
  std::string typeLower = type;
  std::transform(typeLower.begin(), typeLower.end(), 
    typeLower.begin(), ::tolower);
  std::string file = path + timestamp + "." + typeLower + ".iq";

  saveIqFile.open(file, std::ios::binary);

  if (!saveIqFile.is_open())
  {
    std::cerr << "Error: Can not open file: " << file << std::endl;
    exit(1);
  }
  std::cout << "Ready to record IQ to file: " << file << std::endl;

  return file;
}

void Source::close_file()
{
  if (!saveIqFile.is_open())
  {
    saveIqFile.close();
  }

  // switch member with blank file stream
  std::ofstream blankFile;
  std::swap(saveIqFile, blankFile);
}

void Source::kill()
{
  if (type == "RspDuo")
  {
    stop();
  } else if (type == "HackRF" || type == "Kraken")
  {
    stop();
  }
  exit(0);
}
