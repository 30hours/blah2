cmake_minimum_required(VERSION 3.8)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(blah2)
include(CMakePrintHelpers)
include(CTest)

option(BLAH2_KRAKEN_ONLY "Build only the Kraken capture backend" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

find_package(Threads REQUIRED)
find_package(asio  REQUIRED)
find_path(RAPIDJSON_INCLUDE_DIRS "rapidjson/allocators.h")
find_package(ryml CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(Armadillo REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

if(NOT BLAH2_KRAKEN_ONLY)
  set(CMAKE_PREFIX_PATH "/opt/uhd" ${CMAKE_PREFIX_PATH})
  find_package(UHD "4.8.0.0" CONFIG REQUIRED)
endif()

set(PROJECT_ROOT "${PROJECT_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin")
set(PROJECT_BINARY_TEST_DIR "${PROJECT_ROOT}/bin/test")
set(PROJECT_BINARY_TEST_UNIT_DIR "${PROJECT_BINARY_TEST_DIR}/unit")
set(PROJECT_BINARY_TEST_FUNCTIONAL_DIR "${PROJECT_BINARY_TEST_DIR}/functional")
set(PROJECT_BINARY_TEST_COMPARISON_DIR "${PROJECT_BINARY_TEST_DIR}/comparison")

message("Binary path: ${PROJECT_BINARY_DIR}")
message("Binary test path: ${PROJECT_BINARY_TEST_DIR}")

# include from top-level src dir
include_directories(src)
if(NOT BLAH2_KRAKEN_ONLY)
  include_directories(${UHD_INCLUDE_DIRS})
endif()

# TODO: create FindSdrplay.cmake for this
if(NOT BLAH2_KRAKEN_ONLY)
  add_library(sdrplay /usr/local/include/sdrplay_api.h)
  set_target_properties(sdrplay PROPERTIES LINKER_LANGUAGE C)
  target_link_libraries(sdrplay PUBLIC /usr/local/lib/libsdrplay_api.so.3.15)
endif()

set(BLAH2_CAPTURE_SOURCES
  src/capture/kraken/Kraken.cpp
  src/capture/kraken/HeimdallFrame.cpp
)
set(BLAH2_CAPTURE_LIBRARIES)
if(NOT BLAH2_KRAKEN_ONLY)
  list(APPEND BLAH2_CAPTURE_SOURCES
    src/capture/rspduo/RspDuo.cpp
    src/capture/usrp/Usrp.cpp
    src/capture/hackrf/HackRf.cpp
  )
  list(APPEND BLAH2_CAPTURE_LIBRARIES
    ${UHD_LIBRARIES}
    sdrplay
    hackrf
  )
endif()

# TODO: Move to separate src/CMakeLists.txt
add_executable(blah2
  src/blah2.cpp
  src/capture/Capture.cpp
  src/capture/Source.cpp
  ${BLAH2_CAPTURE_SOURCES}
  src/process/ambiguity/Ambiguity.cpp
  src/process/clutter/WienerHopf.cpp
  src/process/conditioning/ArrayReferenceSynthesizer.cpp
  src/process/fusion/Noncoherent.cpp
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
  fftw3
  fftw3_threads
  ${BLAH2_CAPTURE_LIBRARIES}
)
target_include_directories(blah2 PRIVATE RAPIDJSON_INCLUDE_DIRS "rapidjson/allocators.h")
if(BLAH2_KRAKEN_ONLY)
  target_compile_definitions(blah2 PRIVATE BLAH2_KRAKEN_ONLY=1)
endif()

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

add_executable(testKrakenMultiChannel
  test/unit/capture/kraken/TestHeimdallFrame.cpp
  test/unit/process/conditioning/TestArrayReferenceSynthesizer.cpp
  test/unit/process/fusion/TestNoncoherent.cpp
  src/capture/kraken/HeimdallFrame.cpp
  src/process/conditioning/ArrayReferenceSynthesizer.cpp
  src/process/fusion/Noncoherent.cpp
  src/data/IqData.cpp
  src/data/Map.cpp
)
target_link_libraries(testKrakenMultiChannel PRIVATE
  Catch2::Catch2WithMain
)
set_target_properties(testKrakenMultiChannel PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_TEST_UNIT_DIR}")

# TODO: Unsure if will be using CTest.
add_test(NAME testAmbiguity COMMAND testAmbiguity)
add_test(NAME testTracker COMMAND testTracker)
add_test(NAME testHammingNumber COMMAND testHammingNumber)
add_test(NAME testKrakenMultiChannel COMMAND testKrakenMultiChannel)
