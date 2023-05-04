// A real-time radar.

// Author: github.com/30hours
// Date: 31/Jan/2023
// License: MIT

#define RYML_SINGLE_HDR_DEFINE_NOW

#include <ryml-0.5.0.hpp>
#include <asio.hpp>

#include <Capture.h>
#include <Ambiguity.h>
#include <WienerHopf.h>
#include <IqData.h>
#include <Map.h>

#include <sys/types.h>
#include <getopt.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <sys/time.h>
#include <signal.h>
#include <iostream>

void signal_callback_handler(int signum);
void getopt_print_help();
std::string getopt_process(int argc, char **argv);
std::string ryml_get_file(const char *filename);

int main(int argc, char **argv)
{
  // input handling
  signal(SIGINT, signal_callback_handler);
  std::string file = getopt_process(argc, argv);
  std::ifstream filePath(file);
  if (!filePath.is_open())
  {
    std::cout << "Error: Config file does not exist." << std::endl;
    exit(1);
  }

  // config handling
  std::string contents = ryml_get_file(file.c_str());
  ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(contents));

  // setup capture
  uint32_t fs, fc;
  std::string type, path, replayFile;
  bool saveIq, state, loop;
  tree["capture"]["fs"] >> fs;
  tree["capture"]["fc"] >> fc;
  tree["capture"]["type"] >> type;
  tree["save"]["iq"] >> saveIq;
  tree["save"]["path"] >> path;
  tree["capture"]["replay"]["state"] >> state;
  tree["capture"]["replay"]["loop"] >> loop;
  tree["capture"]["replay"]["file"] >> replayFile;
  Capture *capture = new Capture(type, fs, fc, path);
  if (state)
  {
    capture->set_replay(loop, replayFile);
  }

  // create shared queue
  double tBuffer;
  tree["process"]["data"]["buffer"] >> tBuffer;
  IqData *buffer1 = new IqData((int) (tBuffer*fs));
  IqData *buffer2 = new IqData((int) (tBuffer*fs));

  // run capture
  std::thread t1([&]{capture->process(buffer1, buffer2);});

  // setup process CPI
  double tCpi;
  tree["process"]["data"]["cpi"] >> tCpi;
  uint32_t nSamples = fs * tCpi;
  IqData *x = new IqData(nSamples);
  IqData *y = new IqData(nSamples);
  Map<std::complex<double>> *map;
  Map<double> *mapdb;
  std::string mapJson;

  // setup fftw multithread
  if (fftw_init_threads() == 0)
  {
    std::cout << "Error in FFTW multithreading." << std::endl;
    return -1;
  }
  fftw_plan_with_nthreads(4);

  // setup socket
  uint16_t port;
  std::string ip;
  tree["network"]["ports"]["map"] >> port;
  tree["network"]["ip"] >> ip;
  asio::io_service io_service;
  asio::ip::tcp::socket socket(io_service);
  asio::ip::tcp::endpoint endpoint;
  endpoint = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port);
  socket.connect(endpoint);
  asio::error_code err;
  std::string subdata;
  uint32_t MTU = 1024;

  // setup process ambiguity
  int32_t delayMin, delayMax;
  int32_t dopplerMin, dopplerMax;
  tree["process"]["ambiguity"]["delayMin"] >> delayMin;
  tree["process"]["ambiguity"]["delayMax"] >> delayMax;
  tree["process"]["ambiguity"]["dopplerMin"] >> dopplerMin;
  tree["process"]["ambiguity"]["dopplerMax"] >> dopplerMax;
  Ambiguity *ambiguity = new Ambiguity(
    delayMin, delayMax, dopplerMin, dopplerMax, fs, nSamples);

  // setup process clutter
  int32_t delayMinClutter, delayMaxClutter;
  tree["process"]["clutter"]["delayMin"] >> delayMinClutter;
  tree["process"]["clutter"]["delayMax"] >> delayMaxClutter;
  WienerHopf *filter = new WienerHopf(delayMinClutter, delayMaxClutter, nSamples);

  // setup output data
  bool saveMap;
  tree["save"]["map"] >> saveMap;
  std::string savePath, saveMapPath;
  if (saveIq || saveMap)
  {
    char startTimeStr[15];
    struct timeval currentTime = {0, 0};
    gettimeofday(&currentTime, NULL);
    strftime(startTimeStr, 16, "%Y%m%d-%H%M%S", localtime(&currentTime.tv_sec));
    savePath = path + startTimeStr;
  }
  if (saveMap)
  {
    saveMapPath = savePath + ".map";
  }

  // run process
  std::thread t2([&]{
      while (true)
      {
        if ((buffer1->get_length() > nSamples) && (buffer2->get_length() > nSamples))
        {
          // extract data from buffer
          buffer1->set_doNotPush(true);
          buffer2->set_doNotPush(true);
          for (int i = 0; i < nSamples; i++)
          {
            x->push_back(buffer1->pop_front());
            y->push_back(buffer2->pop_front());
          }
          buffer1->set_doNotPush(false);
          buffer2->set_doNotPush(false);

          // radar processing
          if (!filter->process(x, y))
          {
            continue;
          }
          map = ambiguity->process(x, y);

          // output data
          map->set_metrics();
          mapJson = map->to_json();
          if (saveMap)
          {
            map->save(mapJson, saveMapPath);
          }
          for (int i = 0; i < (mapJson.size() + MTU - 1) / MTU; i++)
          {
            subdata = mapJson.substr(i * MTU, MTU);
            socket.write_some(asio::buffer(subdata, subdata.size()), err);
          }

          std::cout << "CPI PROCESSED" << std::endl;
        }
      }
    });

  t1.join();

  return 0;
}

void signal_callback_handler(int signum) {
   std::cout << std::endl;
   std::cout << "Caught signal " << signum << std::endl;
   exit(signum);
}

void getopt_print_help()
{
  std::cout << "--config <file.yml>: 	Set number of program\n"
               "--help:              	Show help\n";
  exit(1);
}

std::string getopt_process(int argc, char **argv)
{
  const char *const short_opts = "c:h";
  const option long_opts[] = {
      {"config", required_argument, nullptr, 'c'},
      {"help", no_argument, nullptr, 'h'},
      {nullptr, no_argument, nullptr, 0}};

  if (argc == 1)
  {
    std::cout << "Error: No arguments provided." << std::endl;
    exit(1);
  }

  std::string file;

  while (true)
  {
    const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

    // handle input "-", ":", etc
    if ((argc == 2) && (-1 == opt))
    {
      std::cout << "Error: No arguments provided." << std::endl;
      exit(1);
    }

    if (-1 == opt)
      break;

    switch (opt)
    {
    case 'c':
      file = std::string(optarg);
      break;

    case 'h':
      getopt_print_help();

    // unrecognised option
    case '?':
      exit(1);

    default:
      break;
    }
  }

  return file;
}

std::string ryml_get_file(const char *filename)
{
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (!in)
  {
    std::cerr << "could not open " << filename << std::endl;
    exit(1);
  }
  std::ostringstream contents;
  contents << in.rdbuf();
  return contents.str();
}