cmake_minimum_required(VERSION 3.8)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(blah2)
include(CMakePrintHelpers)
include(CTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

find_package(Threads REQUIRED)
find_path(RAPIDJSON_INCLUDE_DIRS "rapidjson/allocators.h")
find_package(asio  REQUIRED)
find_package(ryml CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(Armadillo CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(IIO REQUIRED libiio)


set(CMAKE_PREFIX_PATH "/opt/uhd" ${CMAKE_PREFIX_PATH})
find_package(UHD "4.6.0.0" CONFIG REQUIRED)

set(PROJECT_ROOT "${PROJECT_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin")
set(PROJECT_BINARY_TEST_DIR "${PROJECT_ROOT}/bin/test")
set(PROJECT_BINARY_TEST_UNIT_DIR "${PROJECT_BINARY_TEST_DIR}/unit")
set(PROJECT_BINARY_TEST_FUNCTIONAL_DIR "${PROJECT_BINARY_TEST_DIR}/functional")
set(PROJECT_BINARY_TEST_COMPARISON_DIR "${PROJECT_BINARY_TEST_DIR}/comparison")

message("Binary path: ${PROJECT_BINARY_DIR}")
message("Binary test path: ${PROJECT_BINARY_TEST_DIR}")

# include from top-level src dir
include_directories(src ${UHD_INCLUDE_DIRS} ${IIO_INCLUDE_DIRS})

# TODO: create FindSdrplay.cmake for this
add_library(sdrplay /usr/local/include/sdrplay_api.h)
set_target_properties(sdrplay PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(sdrplay PUBLIC /usr/local/lib/libsdrplay_api.so.3.15)

# TODO: Move to separate src/CMakeLists.txt
add_executable(blah2
  src/blah2.cpp
  src/capture/Capture.cpp
  src/capture/Source.cpp
  src/capture/rspduo/RspDuo.cpp
  src/capture/usrp/Usrp.cpp
  src/capture/hackrf/HackRf.cpp
  src/capture/pluto/Pluto.cpp
  src/process/ambiguity/Ambiguity.cpp
  src/process/clutter/WienerHopf.cpp
  src/process/detection/CfarDetector1D.cpp
  src/process/detection/Centroid.cpp
  src/process/detection/Interpolate.cpp
  src/process/tracker/Tracker.cpp
  src/process/spectrum/SpectrumAnalyser.cpp
  src/process/meta/HammingNumber.cpp
  src/process/utility/Socket.cpp
  src/data/IqData.cpp
  src/data/Map.cpp
  src/data/Detection.cpp
  src/data/Track.cpp
  src/data/meta/Timing.cpp
)

target_link_libraries(blah2 PRIVATE 
  Threads::Threads
  asio::asio
  ryml::ryml
  httplib::httplib
  armadillo
  ${UHD_LIBRARIES}
  ${IIO_LIBRARIES}
  fftw3
  fftw3_threads
  sdrplay
  hackrf
)
target_include_directories(blah2 PRIVATE RAPIDJSON_INCLUDE_DIRS "rapidjson/allocators.h")

# unit tests
add_executable(testAmbiguity
  test/unit/process/ambiguity/TestAmbiguity.cpp
  src/data/IqData.cpp
  src/data/Map.cpp
  src/process/ambiguity/Ambiguity.cpp
  src/process/meta/HammingNumber.cpp
)
target_link_libraries(testAmbiguity PRIVATE 
  Catch2::Catch2WithMain 
  fftw3 
  fftw3_threads
)
set_target_properties(testAmbiguity PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_TEST_UNIT_DIR}")

add_executable(testTracker
  test/unit/process/tracker/TestTracker.cpp
  src/data/Detection.cpp
  src/data/Track.cpp
  src/process/tracker/Tracker.cpp
)
target_link_libraries(testTracker PRIVATE 
  Catch2::Catch2WithMain
)
set_target_properties(testTracker PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_TEST_UNIT_DIR}")

add_executable(testHammingNumber
  test/unit/process/meta/TestHammingNumber.cpp
  src/process/meta/HammingNumber.cpp
)
target_link_libraries(testHammingNumber PRIVATE 
  Catch2::Catch2WithMain
)
set_target_properties(testHammingNumber PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_TEST_UNIT_DIR}")

# TODO: Unsure if will be using CTest.
add_test(NAME testAmbiguity COMMAND testAmbiguity)
add_test(NAME testTracker COMMAND testTracker)
