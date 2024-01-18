#include "Usrp.h"

#include <string.h>
#include <iostream>
#include <vector>
#include <complex>
#include <uhd/usrp/multi_usrp.hpp>

// constructor
Usrp::Usrp(std::string _type, uint32_t _fc, uint32_t _fs, 
  std::string _path, bool *_saveIq, std::string _address, 
  std::string _subdev, std::vector<std::string> _antenna, 
  std::vector<double> _gain)
    : Source(_type, _fc, _fs, _path, _saveIq)
{
  address = _address;
  subdev = _subdev;
  antenna = _antenna;
  gain = _gain;
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
      uhd::usrp::multi_usrp::make(address);

    usrp->set_rx_subdev_spec(uhd::usrp::subdev_spec_t(subdev), 0);

    usrp->set_rx_antenna(antenna[0], 0);
    usrp->set_rx_antenna(antenna[1], 1);

    // set sample rate across all channels
    usrp->set_rx_rate((double(fs)));

    // set the center frequency
    double centerFrequency = (double)fc;
    usrp->set_rx_freq(centerFrequency, 0);
    usrp->set_rx_freq(centerFrequency, 1);

    // set the gain
    usrp->set_rx_gain(gain[0], 0);
    usrp->set_rx_gain(gain[1], 1);

    // create a receive streamer
    uhd::stream_args_t streamArgs("fc32", "sc16");
    streamArgs.channels = {0, 1};
    uhd::rx_streamer::sptr rxStreamer = usrp->get_rx_stream(streamArgs);

    // allocate buffers to receive with samples (one buffer per channel)
    const size_t samps_per_buff = rxStreamer->get_max_num_samps();
    std::vector<std::complex<float>> usrpBuffer1(samps_per_buff);
    std::vector<std::complex<float>> usrpBuffer2(samps_per_buff);

    // create a vector of pointers to point to each of the channel buffers
    std::vector<std::complex<float>*> buff_ptrs;
    buff_ptrs.push_back(&usrpBuffer1.front());
    buff_ptrs.push_back(&usrpBuffer2.front());

    // setup stream
    uhd::rx_metadata_t metadata;
    uhd::stream_cmd_t streamCmd = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
    streamCmd.stream_now = false;
    streamCmd.time_spec  = usrp->get_time_now() + uhd::time_spec_t(0.05);
    rxStreamer->issue_stream_cmd(streamCmd);

    while(true)
    {
      // receive samples
      size_t nReceived = rxStreamer->recv(buff_ptrs, samps_per_buff, metadata);

      // print errors
      if (metadata.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
          std::cerr << "Error: " << metadata.strerror() << std::endl;
      }

      buffer1->lock();
      buffer2->lock();
      for (size_t i = 0; i < nReceived; i++)
      {
        buffer1->push_back({(double)buff_ptrs[0][i].real(), (double)buff_ptrs[0][i].imag()});
        buffer2->push_back({(double)buff_ptrs[1][i].real(), (double)buff_ptrs[1][i].imag()});
      }
      buffer1->unlock();
      buffer2->unlock();

      sleep(0.01);
    }
}

void Usrp::replay(IqData *buffer1, IqData *buffer2, std::string _file, bool _loop)
{
  return;
}

void Usrp::open_file()
{
  return;
}

void Usrp::close_file()
{
  return;
}
