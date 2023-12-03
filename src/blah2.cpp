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
#include <Centroid.h>
#include <Interpolate.h>
#include <Timing.h>
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
uint64_t current_time_ms();
uint64_t current_time_us();

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
  Detection *detection1;
  Detection *detection2;

  // setup fftw multithread
  if (fftw_init_threads() == 0)
  {
    std::cout << "Error in FFTW multithreading." << std::endl;
    return -1;
  }
  fftw_plan_with_nthreads(4);

  // setup socket
  uint16_t port_map, port_detection, port_timestamp, port_timing;
  std::string ip;
  tree["network"]["ports"]["map"] >> port_map;
  tree["network"]["ports"]["detection"] >> port_detection;
  tree["network"]["ports"]["timestamp"] >> port_timestamp;
  tree["network"]["ports"]["timing"] >> port_timing;
  tree["network"]["ip"] >> ip;
  asio::io_service io_service;
  asio::ip::tcp::socket socket_map(io_service);
  asio::ip::tcp::socket socket_detection(io_service);
  asio::ip::tcp::socket socket_timestamp(io_service);
  asio::ip::tcp::socket socket_timing(io_service);
  asio::ip::tcp::endpoint endpoint_map;
  asio::ip::tcp::endpoint endpoint_detection;
  asio::ip::tcp::endpoint endpoint_timestamp;
  asio::ip::tcp::endpoint endpoint_timing;
  endpoint_map = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port_map);
  endpoint_detection = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port_detection);
  endpoint_timestamp = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port_timestamp);
  endpoint_timing = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port_timing);
  socket_map.connect(endpoint_map);
  socket_detection.connect(endpoint_detection);
  socket_timestamp.connect(endpoint_timestamp);
  socket_timing.connect(endpoint_timing);
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
  double pfa, minDoppler;
  int8_t nGuard, nTrain;
  int8_t minDelay;
  tree["process"]["detection"]["pfa"] >> pfa;
  tree["process"]["detection"]["nGuard"] >> nGuard;
  tree["process"]["detection"]["nTrain"] >> nTrain;
  tree["process"]["detection"]["minDelay"] >> minDelay;
  tree["process"]["detection"]["minDoppler"] >> minDoppler;
  CfarDetector1D *cfarDetector1D = new CfarDetector1D(pfa, nGuard, nTrain, minDelay, minDoppler);
  Interpolate *interpolate = new Interpolate(true, true);

  // setup process centroid
  uint16_t nCentroid;
  tree["process"]["detection"]["nCentroid"] >> nCentroid;
  Centroid *centroid = new Centroid(nCentroid, nCentroid, 1/tCpi);

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

  // setup output timing
  uint64_t tStart = current_time_ms();
  Timing *timing = new Timing(tStart);
  std::vector<std::string> timing_name;
  std::vector<double> timing_time;
  std::string jsonTiming;

  // run process
  std::thread t2([&]{
      while (true)
      {
        if ((buffer1->get_length() > nSamples) && (buffer2->get_length() > nSamples))
        {
          uint64_t t0 = current_time_us();
          
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
          uint64_t t1 = current_time_us();
          double delta_t1 = (double)(t1-t0) / 1000;
          timing_name.push_back("extract_buffer");
          timing_time.push_back(delta_t1);

          // clutter filter
          if (!filter->process(x, y))
          {
            continue;
          }
          uint64_t t2 = current_time_us();
          double delta_t2 = (double)(t2-t1) / 1000;
          timing_name.push_back("clutter_filter");
          timing_time.push_back(delta_t2);

          // ambiguity process
          map = ambiguity->process(x, y);
          map->set_metrics();
          uint64_t t3 = current_time_us();
          double delta_t3 = (double)(t3-t2) / 1000;
          timing_name.push_back("ambiguity_processing");
          timing_time.push_back(delta_t3);

          // detection process
          detection1 = cfarDetector1D->process(map);
          detection2 = centroid->process(detection1);
          detection = interpolate->process(detection2, map);
          
          uint64_t t4 = current_time_us();
          double delta_t4 = (double)(t4-t3) / 1000;
          timing_name.push_back("detector");
          timing_time.push_back(delta_t4);

          // output map data
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

          // output detection data
          detectionJson = detection->to_json();
          detectionJson = detection->delay_bin_to_km(detectionJson, fs);
          for (int i = 0; i < (detectionJson.size() + MTU - 1) / MTU; i++)
          {
            subdata = detectionJson.substr(i * MTU, MTU);
            socket_detection.write_some(asio::buffer(subdata, subdata.size()), err);
          }
          delete detection;
          delete detection1;
          delete detection2;
          
          // output radar data timer
          uint64_t t5 = current_time_us();
          double delta_t5 = (double)(t5-t4) / 1000;
          timing_name.push_back("output_radar_data");
          timing_time.push_back(delta_t5);

          // cpi timer
          uint64_t t6 = current_time_us();
          double delta_t6 = (double)(t6-t0) / 1000;
          timing_name.push_back("cpi");
          timing_time.push_back(delta_t6);
          std::cout << "CPI time (ms): " << delta_t6 << std::endl;

          // output timing data
          timing->update(t0/1000, timing_time, timing_name);
          jsonTiming = timing->to_json();
          socket_timing.write_some(asio::buffer(jsonTiming, 1500), err);
          timing_time.clear();
          timing_name.clear();

          // output CPI timestamp for updating data
          std::string t0_string = std::to_string(t0);
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

uint64_t current_time_ms()
{
  // current time in POSIX ms
  return std::chrono::duration_cast<std::chrono::milliseconds>
  (std::chrono::system_clock::now().time_since_epoch()).count();
}

uint64_t current_time_us()
{
  // current time in POSIX us
  return std::chrono::duration_cast<std::chrono::microseconds>
  (std::chrono::system_clock::now().time_since_epoch()).count();
}