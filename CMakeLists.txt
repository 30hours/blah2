cmake_minimum_required(VERSION 3.8)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)
  
project(blah2)
include(CMakePrintHelpers)
include(CTest)

find_package(Threads REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(ryml CONFIG REQUIRED)
find_package(httplib REQUIRED)
find_package(Armadillo REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

# TODO: When release CI is finished, don't use these dirs, install target should go to prod
SET (PROJECT_ROOT "${PROJECT_SOURCE_DIR}")
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin")
SET (PROJECT_BINARY_TEST_DIR "${PROJECT_ROOT}/bin/test")

MESSAGE ("Binary path: ${PROJECT_BINARY_DIR}")
MESSAGE ("Binary test path: ${PROJECT_BINARY_TEST_DIR}")

# Include from top-level src dir
include_directories(src)

# TODO: Create FindSdrplay.cmake for this
add_library(sdrplay /usr/local/include/sdrplay_api.h)
set_target_properties(sdrplay PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(sdrplay PUBLIC /usr/local/lib/libsdrplay_api.so.3.07)

add_executable(blah2
  src/blah2.cpp
  src/capture/Capture.cpp
  src/capture/rspduo/RspDuo.cpp
  src/process/ambiguity/Ambiguity.cpp
  src/process/clutter/WienerHopf.cpp
  src/process/detection/CfarDetector1D.cpp
  src/process/detection/Centroid.cpp
  src/process/detection/Interpolate.cpp
  src/process/tracker/Tracker.cpp
  src/process/spectrum/SpectrumAnalyser.cpp
  src/process/meta/HammingNumber.cpp
  src/data/IqData.cpp
  src/data/Map.cpp
  src/data/Detection.cpp
  src/data/Track.cpp
  src/data/meta/Timing.cpp
)

target_link_libraries(blah2 PRIVATE 
  Threads::Threads
  rapidjson
  asio::asio
  ryml::ryml
  httplib::httplib
  armadillo
  fftw3
  fftw3_threads
  sdrplay
)

# unit tests
add_executable(testAmbiguity
  test/unit/process/ambiguity/TestAmbiguity.cpp
  src/data/IqData.cpp
  src/data/Map.cpp
  src/process/ambiguity/Ambiguity.cpp
  src/process/meta/HammingNumber.cpp)
target_link_libraries(testAmbiguity PRIVATE Catch2::Catch2WithMain fftw3 fftw3_threads)
set_target_properties(testAmbiguity PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_TEST_UNIT_DIR}")

add_executable(testTracker
  test/unit/process/tracker/TestTracker.cpp
  src/data/Detection.cpp
  src/data/Track.cpp
  src/process/tracker/Tracker.cpp)
target_link_libraries(testTracker PRIVATE Catch2::Catch2WithMain)
set_target_properties(testTracker PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_TEST_UNIT_DIR}")

add_test(NAME testAmbiguity COMMAND testAmbiguity)
add_test(NAME testTracker COMMAND testTracker)
