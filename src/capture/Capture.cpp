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
}

void Capture::process(IqData *buffer1, IqData *buffer2)
{
  if (is_type_valid(type))
  {
    std::cout << "Setting up device " + type << std::endl;

    // RspDuo device
    if (type == VALID_TYPE[0])
    {
      RspDuo *device = new RspDuo(this->fc, this->path);
    }

    // Usrp device
    if (type == VALID_TYPE[1])
    {
      //Usrp *device = new Usrp(this->fc, this->path);
    }
    Usrp *device = new Usrp(this->fc, this->path);

    // capture status thread
    std::thread t1([&]{
      while (true)
      {
        httplib::Client cli("http://127.0.0.1:3000");
        httplib::Result res = cli.Get("/capture");

        // if capture status changed
        if ((res->body == "true") != device->get_capture())
        {
          device->set_capture(res->body == "true");
          if (device->get_capture())
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
  }
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