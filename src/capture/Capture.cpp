#include "Capture.h"
#include "RspDuo.h"
#include <iostream>
#include <thread>
#include <httplib.h>

// constants
const std::string Capture::VALID_TYPE[2] = {"RspDuo", "HackRF"};

// constructor
Capture::Capture(std::string _type, uint32_t _fs, uint32_t _fc, std::string _path)
{
  type = _type;
  fs = _fs;
  fc = _fc;
  path = _path;
}

void Capture::process(IqData *buffer1, IqData *buffer2)
{
  // case RspDuo
  if (type == Capture::VALID_TYPE[0])
  {
    std::cout << "Setting up device " + type << std::endl;
    RspDuo *rspDuo = new RspDuo(this->fc, this->path);

    // capture status thread
    std::thread t1([&]{
      while (true)
      {
        httplib::Client cli("http://127.0.0.1:3000");
        httplib::Result res = cli.Get("/capture");

        // if capture status changed
        if ((res->body == "true") != rspDuo->get_capture())
        {
          rspDuo->set_capture(res->body == "true");
          if (rspDuo->get_capture())
          {
            rspDuo->open_file();
          }
          else
          {
            rspDuo->close_file();
          }
        }

        sleep(1);
      }
    });

    if (!replay)
    {
      rspDuo->start();
      rspDuo->process(buffer1, buffer2);
    }
    else
    {
      rspDuo->replay(buffer1, buffer2, file, loop);
    }
  }
  // case HackRF
  else if (type == Capture::VALID_TYPE[1])
  {
  }
  else
  {
    std::cout << "Error: Invalid capture device" << std::endl;
    exit(1);
  }
}

void Capture::set_replay(bool _loop, std::string _file)
{
  replay = true;
  loop = _loop;
  file = _file;
}