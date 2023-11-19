/// @file blah2.cpp
/// @brief A real-time radar.
/// @author 30hours

#define RYML_SINGLE_HDR_DEFINE_NOW

#include <ryml-0.5.0.hpp>
#include <asio.hpp>
#include <Capture.h>
#include <Ambiguity.h>
#include <WienerHopf.h>
#include <CfarDetector1D.h>
#include <IqData.h>
#include <Map.h>
#include <Detection.h>
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
  std::string mapJson, detectionJson;
  Detection *detection;

  // setup fftw multithread
  if (fftw_init_threads() == 0)
  {
    std::cout << "Error in FFTW multithreading." << std::endl;
    return -1;
  }
  fftw_plan_with_nthreads(4);

  // setup socket
  uint16_t port_map, port_detection, port_timestamp;
  std::string ip;
  tree["network"]["ports"]["map"] >> port_map;
  tree["network"]["ports"]["detection"] >> port_detection;
  tree["network"]["ports"]["timestamp"] >> port_timestamp;
  tree["network"]["ip"] >> ip;
  asio::io_service io_service;
  asio::ip::tcp::socket socket_map(io_service);
  asio::ip::tcp::socket socket_detection(io_service);
  asio::ip::tcp::socket socket_timestamp(io_service);
  asio::ip::tcp::endpoint endpoint_map;
  asio::ip::tcp::endpoint endpoint_detection;
  asio::ip::tcp::endpoint endpoint_timestamp;
  endpoint_map = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port_map);
  endpoint_detection = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port_detection);
  endpoint_timestamp = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port_timestamp);
  socket_map.connect(endpoint_map);
  socket_detection.connect(endpoint_detection);
  socket_timestamp.connect(endpoint_timestamp);
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

  // setup process detection
  double pfa;
  int8_t nGuard, nTrain;
  tree["process"]["detection"]["pfa"] >> pfa;
  tree["process"]["detection"]["nGuard"] >> nGuard;
  tree["process"]["detection"]["nTrain"] >> nTrain;
  CfarDetector1D *cfarDetector1D = new CfarDetector1D(pfa, nGuard, nTrain);

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
          auto t0 = std::chrono::high_resolution_clock::now();
          
          // extract data from buffer
          buffer1->lock();
          buffer2->lock();
          for (int i = 0; i < nSamples; i++)
          {
            x->push_back(buffer1->pop_front());
            y->push_back(buffer2->pop_front());
          }
          buffer1->unlock();
          buffer2->unlock();
          auto t1 = std::chrono::high_resolution_clock::now();
          double delta_t1 = std::chrono::duration<double, std::milli>(t1-t0).count();
          std::cout << "Extract data from buffer (ms): " << delta_t1 << std::endl;

          // clutter filter
          if (!filter->process(x, y))
          {
            continue;
          }
          auto t2 = std::chrono::high_resolution_clock::now();
          double delta_t2 = std::chrono::duration<double, std::milli>(t2-t1).count();
          std::cout << "Clutter filter (ms): " << delta_t2 << std::endl;

          // ambiguity process
          map = ambiguity->process(x, y);
          auto t3 = std::chrono::high_resolution_clock::now();
          double delta_t3 = std::chrono::duration<double, std::milli>(t3-t2).count();
          std::cout << "Ambiguity processing (ms): " << delta_t3 << std::endl;

          // detection process
          // detection = cfarDetector1D->process(map);
          auto t4 = std::chrono::high_resolution_clock::now();
          double delta_t4 = std::chrono::duration<double, std::milli>(t4-t3).count();
          std::cout << "Detection processing (ms): " << delta_t4 << std::endl;

          // output map data
          map->set_metrics();
          mapJson = map->to_json();
          mapJson = map->delay_bin_to_km(mapJson, fs);
          if (saveMap)
          {
            map->save(mapJson, saveMapPath);
          }
          for (int i = 0; i < (mapJson.size() + MTU - 1) / MTU; i++)
          {
            subdata = mapJson.substr(i * MTU, MTU);
            socket_map.write_some(asio::buffer(subdata, subdata.size()), err);
          }
          auto t5 = std::chrono::high_resolution_clock::now();
          double delta_t5 = std::chrono::duration<double, std::milli>(t5-t4).count();
          std::cout << "Output map data (ms): " << delta_t5 << std::endl;

          // output detection data
          // detectionJson = detection->to_json();
          // for (int i = 0; i < (detectionJson.size() + MTU - 1) / MTU; i++)
          // {
          //   subdata = detectionJson.substr(i * MTU, MTU);
          //   socket_detection.write_some(asio::buffer(subdata, subdata.size()), err);
          // }
          // delete detection;
          auto t6 = std::chrono::high_resolution_clock::now();
          double delta_t6 = std::chrono::duration<double, std::milli>(t6-t5).count();
          std::cout << "Output detection data (ms): " << delta_t6 << std::endl;

          auto t7 = std::chrono::high_resolution_clock::now();
          double delta_t7 = std::chrono::duration<double, std::milli>(t7-t0).count();
          std::cout << "CPI time (ms): " << delta_t7 << std::endl;

          // output CPI timestamp for updating data
          auto t0_duration = t0.time_since_epoch();
          auto t0_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t0_duration).count();
          std::string t0_string = std::to_string(t0_ms);
          socket_timestamp.write_some(asio::buffer(t0_string, 100), err);
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