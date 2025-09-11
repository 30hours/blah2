#include "Capture.h"
#include "rspduo/RspDuo.h"
#include <iostream>
#include <thread>
#include <httplib.h>

// constants
const std::string Capture::VALID_TYPE[1] = {"RspDuo"};

// constructor
Capture::Capture(std::string _type, uint32_t _fs, uint32_t _fc, std::string _path)
{
  type = _type;
  fs = _fs;
  fc = _fc;
  path = _path;
  replay = false;
  saveIq = false;
}

void Capture::process(IqData *buffer1, IqData *buffer2, c4::yml::NodeRef config, 
  std::string ip_capture, uint16_t port_capture)
{
  std::cout << "Setting up device " + type << std::endl;

  device = factory_source(type, config);

  // capture status thread
  std::thread t1([&]{
    while (true)
    {
      httplib::Client cli("http://" + ip_capture + ":" 
        + std::to_string(port_capture));
      httplib::Result res = cli.Get("/capture");

      // if capture status changed
      if ((res->body == "true") != saveIq)
      {
        saveIq = res->body == "true";
        if (saveIq)
        {
          device->open_file();
        }
        else
        {
          device->close_file();
        }
      }
      sleep(1);
    }
  });

  if (!replay)
  {
    device->start();
    device->process(buffer1, buffer2);
  }
  else
  {
    device->replay(buffer1, buffer2, file, loop);
  }
  t1.join();
}

std::unique_ptr<Source> Capture::factory_source(const std::string& type, c4::yml::NodeRef config)
{
    // SDRplay RSPduo
    if (type == VALID_TYPE[0])
    {
        int agcSetPoint, bandwidthNumber, gainReduction, lnaState;
        bool dabNotch, rfNotch;
        config["agcSetPoint"] >> agcSetPoint;
        config["bandwidthNumber"] >> bandwidthNumber;
        config["gainReduction"] >> gainReduction;
        config["lnaState"] >> lnaState;
        config["dabNotch"] >> dabNotch;
        config["rfNotch"] >> rfNotch;
        return std::make_unique<RspDuo>(type, fc, fs, path, &saveIq,
          agcSetPoint, bandwidthNumber, gainReduction, lnaState,
          dabNotch, rfNotch);
    }

    // handle unknown type
    std::cerr << "Error: Source type does not exist." << std::endl;
    return nullptr;
}

void Capture::set_replay(bool _loop, std::string _file)
{
  replay = true;
  loop = _loop;
  file = _file;
}
