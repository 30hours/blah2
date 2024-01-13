#include "Usrp.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <iostream>

#include <uhd/usrp/multi_usrp.hpp>
#include <complex>

// constructor
Usrp::Usrp(uint32_t _fc, std::string _path)
{
  fc = _fc;
  path = _path;
  capture = false;
}

std::string Usrp::set_file(std::string path)
{
  // todo: deprecate
  return "/blah2/tmp.iq";
}

void Usrp::start()
{
}

void Usrp::stop()
{
}

void Usrp::process(IqData *buffer1, IqData *buffer2)
{
    // create a USRP object
    uhd::usrp::multi_usrp::sptr usrp = 
      uhd::usrp::multi_usrp::make("localhost");

    // Set the sample rate
    double sampleRate = 2e6; // Replace with your desired sample rate
    usrp->set_rx_rate(sampleRate);

    // Set the center frequency
    double centerFrequency = (double)fc;
    usrp->set_rx_freq(centerFrequency);

    // Set the gain
    double gain = 20.0; // Replace with your desired gain
    usrp->set_rx_gain(gain);

    // Set the number of channels
    size_t numChannels = 1; // Assuming one channel for simplicity
    usrp->set_rx_antenna("RX2", 0); // Set antenna for channel 0

    // Set the receive buffer size
    size_t bufferSize = 1024 * numChannels;
    std::vector<std::complex<float>> buffer(bufferSize);

    // Create a receive streamer
    uhd::stream_args_t streamArgs("fc32", "sc16");
    uhd::rx_streamer::sptr rxStreamer = usrp->get_rx_stream(streamArgs);

    // Setup streaming
    uhd::rx_metadata_t metadata;
    rxStreamer->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

    while(true)
    {
      // Receive samples
      size_t numReceived = rxStreamer->recv(&buffer[0], buffer.size(), metadata);

      // Stop streaming
      // rxStreamer->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);

      // Check for errors
      if (metadata.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
          std::cerr << "Error during reception: " << metadata.strerror() << std::endl;
          return;
      }

      // Copy received samples to the output buffer
      //iqBuffer.resize(numReceived);
      //std::copy(buffer.begin(), buffer.begin() + numReceived, iqBuffer.begin());
      buffer1->lock();
      buffer2->lock();
      for (size_t i = 0; i < buffer.size(); i++)
      {
        buffer1->push_back({(double)buffer[i].real(), (double)buffer[i].imag()});
        buffer2->push_back({(double)buffer[i].real(), (double)buffer[i].imag()});
      }
      buffer1->unlock();
      buffer2->unlock();
    }
}

void Usrp::replay(IqData *buffer1, IqData *buffer2, std::string _file, bool _loop)
{
  short i1, q1, i2, q2;
  int rv;
  FILE *file_replay = fopen(_file.c_str(), "rb");

  while (true)
  {
    rv = fread(&i1, 1, sizeof(short), file_replay);
    rv = fread(&q1, 1, sizeof(short), file_replay);
    rv = fread(&i2, 1, sizeof(short), file_replay);
    rv = fread(&q2, 1, sizeof(short), file_replay);

    buffer1->lock();
    buffer2->lock();

    if (buffer1->get_length() < buffer1->get_n())
    {
      buffer1->push_back({(double)i1, (double)q1});
      buffer2->push_back({(double)i2, (double)q2});
    }

    buffer1->unlock();
    buffer2->unlock();

  }

}

void Usrp::open_file()
{
  return;
}

void Usrp::close_file()
{
  return;
}


void Usrp::set_capture(bool _capture)
{
  capture = _capture;
}

bool Usrp::get_capture()
{
  return capture;
}
