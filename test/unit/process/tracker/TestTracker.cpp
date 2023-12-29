/// @file TestTracker.cpp
/// @brief Unit test for Tracker.cpp
/// @author 30hours

#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"

#include "Detection.h"
#include "Tracker.h"
#include "Track.h"
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
    double cpi = 0.5;
    double maxAccInit = 10;
    double rangeRes = 100;
    Tracker tracker = Tracker(m, n, nDelete, cpi, maxAccInit, rangeRes);
}

/// @brief Test process for an ACTIVE track.
TEST_CASE("Process ACTIVE track constant acc", "[process]")
{
  uint32_t m = 3;
  uint32_t n = 5;
  uint32_t nDelete = 5;
  double cpi = 0.5;
  double maxAccInit = 10;
  double rangeRes = 100;
  Tracker tracker = Tracker(m, n, nDelete, 
    cpi, maxAccInit, rangeRes);
  
  
  // create detections with constant acc 5 Hz/s
  std::vector<double> delay = {10};
  std::vector<double> doppler = {-20};

  std::string state = "ACTIVE";
}