#include "Capture.h"
#include "rspduo/RspDuo.h"
#include "usrp/Usrp.h"
#include <iostream>
#include <thread>
#include <httplib.h>

// constants
const std::string Capture::VALID_TYPE[2] = {"RspDuo", "Usrp"};

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

void Capture::process(IqData *buffer1, IqData *buffer2, c4::yml::NodeRef config)
{
  std::cout << "Setting up device " + type << std::endl;

  std::unique_ptr<Source> device = factory_source(type, config);

  if (!replay)
  {
    device->start();
    device->process(buffer1, buffer2);
  }
  else
  {
    device->replay(buffer1, buffer2, file, loop);
  }

  // capture status thread
  std::thread t1([&]{
    while (true)
    {
      httplib::Client cli("http://127.0.0.1:3000");
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
}

std::unique_ptr<Source> Capture::factory_source(const std::string& type, c4::yml::NodeRef config)
{
    if (type == VALID_TYPE[0])
    {
        return std::make_unique<RspDuo>(type, fc, fs, path, &saveIq);
    }
    else if (type == VALID_TYPE[1])
    {
        std::string address, subdev;
        std::vector<std::string> antenna;
        std::vector<double> gain;
        std::string _antenna;
        double _gain;
        config["address"] >> address;
        config["subdev"] >> subdev;
        config["antenna"][0] >> _antenna;
        antenna.push_back(_antenna);
        config["antenna"][1] >> _antenna;
        antenna.push_back(_antenna);
        config["gain"][0] >> _gain;
        gain.push_back(_gain);
        config["gain"][1] >> _gain;
        gain.push_back(_gain);
        
        return std::make_unique<Usrp>(type, fc, fs, path, &saveIq, 
          address, subdev, antenna, gain);
    }
    // Handle unknown type
    std::cerr << "Error: Source type does not exist." << std::endl;
    return nullptr;
}

void Capture::set_replay(bool _loop, std::string _file)
{
  replay = true;
  loop = _loop;
  file = _file;
}

bool Capture::is_type_valid(std::string _type)
{
  size_t n = sizeof(Capture::VALID_TYPE) / 
    sizeof(Capture::VALID_TYPE[0]);
  for (size_t i = 0; i < n; i++)
  {
    if (_type == Capture::VALID_TYPE[i])
    {
      return true;
    }
  }
  std::cerr << "Invalid capture device: " << _type << std::endl;
  return false;
}