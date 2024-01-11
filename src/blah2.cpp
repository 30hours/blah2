/// @file blah2.cpp
/// @brief A real-time radar.
/// @author 30hours

#include "capture/Capture.h"
#include "data/IqData.h"
#include "data/Map.h"
#include "data/Detection.h"
#include "data/meta/Timing.h"
#include "data/Track.h"
#include "process/ambiguity/Ambiguity.h"
#include "process/clutter/WienerHopf.h"
#include "process/detection/CfarDetector1D.h"
#include "process/detection/Centroid.h"
#include "process/detection/Interpolate.h"
#include "process/spectrum/SpectrumAnalyser.h"
#include "process/tracker/Tracker.h"

#include <ryml/ryml.hpp>
#include <ryml/ryml_std.hpp> // optional header, provided for std:: interop
#include <c4/format.hpp> // needed for the examples below
#include <asio.hpp>
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
void timing_helper(std::vector<std::string>& timing_name, 
  std::vector<double>& timing_time, std::vector<uint64_t>& time_us, 
  std::string name);

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
  Detection *detection;
  Detection *detection1;
  Detection *detection2;
  Track *track;

  // setup fftw multithread
  if (fftw_init_threads() == 0)
  {
    std::cout << "Error in FFTW multithreading." << std::endl;
    return -1;
  }
  fftw_plan_with_nthreads(4);

  // setup socket
  uint16_t port_map, port_detection, port_timestamp, 
    port_timing, port_iqdata, port_track;
  std::string ip;
  tree["network"]["ports"]["map"] >> port_map;
  tree["network"]["ports"]["detection"] >> port_detection;
  tree["network"]["ports"]["track"] >> port_track;
  tree["network"]["ports"]["timestamp"] >> port_timestamp;
  tree["network"]["ports"]["timing"] >> port_timing;
  tree["network"]["ports"]["iqdata"] >> port_iqdata;
  tree["network"]["ip"] >> ip;
  asio::io_service io_service;
  asio::ip::tcp::socket socket_map(io_service);
  asio::ip::tcp::socket socket_detection(io_service);
  asio::ip::tcp::socket socket_track(io_service);
  asio::ip::tcp::socket socket_timestamp(io_service);
  asio::ip::tcp::socket socket_timing(io_service);
  asio::ip::tcp::socket socket_iqdata(io_service);
  asio::ip::tcp::endpoint endpoint_map;
  asio::ip::tcp::endpoint endpoint_detection;
  asio::ip::tcp::endpoint endpoint_track;
  asio::ip::tcp::endpoint endpoint_timestamp;
  asio::ip::tcp::endpoint endpoint_timing;
  asio::ip::tcp::endpoint endpoint_iqdata;
  endpoint_map = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port_map);
  endpoint_detection = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port_detection);
  endpoint_track = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port_track);
  endpoint_timestamp = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port_timestamp);
  endpoint_timing = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port_timing);
  endpoint_iqdata = asio::ip::tcp::endpoint(
    asio::ip::address::from_string(ip), port_iqdata);
  socket_map.connect(endpoint_map);
  socket_detection.connect(endpoint_detection);
  socket_track.connect(endpoint_track);
  socket_timestamp.connect(endpoint_timestamp);
  socket_timing.connect(endpoint_timing);
  socket_iqdata.connect(endpoint_iqdata);
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

  // setup process tracker
  uint8_t m, n, nDelete;
  double maxAcc, rangeRes, lambda;
  std::string smooth;
  tree["process"]["tracker"]["initiate"]["M"] >> m;
  tree["process"]["tracker"]["initiate"]["N"] >> n;
  tree["process"]["tracker"]["delete"] >> nDelete;
  tree["process"]["tracker"]["initiate"]["maxAcc"] >> maxAcc;
  rangeRes = 299792458.0/fs;
  lambda = 299792458.0/fc;
  Tracker *tracker = new Tracker(m, n, nDelete, ambiguity->cpi_length_seconds(), maxAcc, rangeRes, lambda);

  // setup process spectrum analyser
  double spectrumBandwidth = 2000;
  SpectrumAnalyser *spectrumAnalyser = new SpectrumAnalyser(nSamples, spectrumBandwidth);

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
  std::vector<uint64_t> time;

  // setup output json
  std::string mapJson, detectionJson, jsonTracker, jsonIqData;

  // run process
  std::thread t2([&]{
      while (true)
      {
        if ((buffer1->get_length() > nSamples) && (buffer2->get_length() > nSamples))
        {
          time.push_back(current_time_us());
          
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
          timing_helper(timing_name, timing_time, time, "extract_buffer");
          
          // spectrum
          spectrumAnalyser->process(x);
          timing_helper(timing_name, timing_time, time, "spectrum");
          
          // clutter filter
          if (!filter->process(x, y))
          {
            continue;
          }
          timing_helper(timing_name, timing_time, time, "clutter_filter");
          
          // ambiguity process
          map = ambiguity->process(x, y);
          map->set_metrics();
          timing_helper(timing_name, timing_time, time, "ambiguity_processing");
          
          // detection process
          detection1 = cfarDetector1D->process(map);
          detection2 = centroid->process(detection1);
          detection = interpolate->process(detection2, map);
          timing_helper(timing_name, timing_time, time, "detector");

          // tracker process
          track = tracker->process(detection, time[0]/1000);
          timing_helper(timing_name, timing_time, time, "tracker");

          // output IqData meta data
          jsonIqData = x->to_json(time[0]/1000);
          for (int i = 0; i < (jsonIqData.size() + MTU - 1) / MTU; i++)
          {
            subdata = jsonIqData.substr(i * MTU, MTU);
            socket_iqdata.write_some(asio::buffer(subdata, subdata.size()), err);
          }

          // output map data
          mapJson = map->to_json(time[0]/1000);
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
          detectionJson = detection->to_json(time[0]/1000);
          detectionJson = detection->delay_bin_to_km(detectionJson, fs);
          for (int i = 0; i < (detectionJson.size() + MTU - 1) / MTU; i++)
          {
            subdata = detectionJson.substr(i * MTU, MTU);
            socket_detection.write_some(asio::buffer(subdata, subdata.size()), err);
          }
          delete detection;
          delete detection1;
          delete detection2;

          // output tracker data
          jsonTracker = track->to_json(time[0]/1000);
          for (int i = 0; i < (jsonTracker.size() + MTU - 1) / MTU; i++)
          {
            subdata = jsonTracker.substr(i * MTU, MTU);
            socket_track.write_some(asio::buffer(subdata, subdata.size()), err);
          }

          // output radar data timer
          timing_helper(timing_name, timing_time, time, "output_radar_data");

          // cpi timer
          time.push_back(current_time_us());
          double delta_ms = (double)(time.back()-time[0]) / 1000;
          timing_name.push_back("cpi");
          timing_time.push_back(delta_ms);
          std::cout << "CPI time (ms): " << delta_ms << std::endl;

          // output timing data
          timing->update(time[0]/1000, timing_time, timing_name);
          jsonTiming = timing->to_json();
          socket_timing.write_some(asio::buffer(jsonTiming, 1500), err);
          timing_time.clear();
          timing_name.clear();

          // output CPI timestamp for updating data
          std::string t0_string = std::to_string(time[0]/1000);
          socket_timestamp.write_some(asio::buffer(t0_string, 100), err);
          time.clear();
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

void timing_helper(std::vector<std::string>& timing_name, 
  std::vector<double>& timing_time, std::vector<uint64_t>& time_us, 
  std::string name)
{
  time_us.push_back(current_time_us());
  double delta_ms = (double)(time_us.back()-time_us[time_us.size()-2]) / 1000;
  timing_name.push_back(name);
  timing_time.push_back(delta_ms);
}