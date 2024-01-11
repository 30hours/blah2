/// @file TestTracker.cpp
/// @brief Unit test for Tracker.cpp
/// @author 30hours

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "data/Detection.h"
#include "data/Track.h"
#include "process/tracker/Tracker.h"

#include <string>
#include <vector>
#include <random>
#include <iostream>

/// @brief Test constructor.
/// @details Check constructor parameters created correctly.
TEST_CASE("Constructor", "[constructor]")
{
  uint32_t m = 3;
  uint32_t n = 5;
  uint32_t nDelete = 5;
  double cpi = 1;
  double maxAccInit = 10;
  double fs = 2000000;
  double rangeRes = 299792458.0/fs;
  double fc = 204640000;
  double lambda = 299792458.0/fc;
  Tracker tracker = Tracker(m, n, nDelete, 
    cpi, maxAccInit, rangeRes, lambda);
}

/// @brief Test process for an ACTIVE track.
TEST_CASE("Process ACTIVE track constant acc", "[process]")
{
  uint32_t m = 3;
  uint32_t n = 5;
  uint32_t nDelete = 5;
  double cpi = 1;
  double maxAccInit = 10;
  double fs = 2000000;
  double rangeRes = 299792458.0/fs;
  double fc = 204640000;
  double lambda = 299792458.0/fc;
  Tracker tracker = Tracker(m, n, nDelete, 
    cpi, maxAccInit, rangeRes, lambda);
  
  
  // create detections with constant acc 5 Hz/s
  std::vector<uint64_t> timestamp = {0,1,2,3,4,5,6,7,8,9,10};
  std::vector<double> delay = {10};
  std::vector<double> doppler = {-20,-15,-10,-5,0,5,10,15,20,25};

  std::string state = "ACTIVE";
}

/// @brief Test predict for kinematics equations.
TEST_CASE("Test predict", "[predict]")
{
  uint32_t m = 3;
  uint32_t n = 5;
  uint32_t nDelete = 5;
  double cpi = 1;
  double maxAccInit = 10;
  double fs = 2000000;
  double rangeRes = 299792458.0/fs;
  double fc = 204640000;
  double lambda = 299792458.0/fc;
  Tracker tracker = Tracker(m, n, nDelete, 
    cpi, maxAccInit, rangeRes, lambda);

  Detection input = Detection(10, -20, 0);
  double acc = 5;
  double T = 1;
  Detection prediction = tracker.predict(input, acc, T);
  Detection prediction_truth = Detection(9.821, -15, 0);

  CHECK_THAT(prediction.get_delay().front(), 
    Catch::Matchers::WithinAbs(prediction_truth.get_delay().front(), 0.01));
  CHECK_THAT(prediction.get_doppler().front(), 
    Catch::Matchers::WithinAbs(prediction_truth.get_doppler().front(), 0.01));
}