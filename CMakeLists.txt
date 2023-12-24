cmake_minimum_required(VERSION 3.8)
  if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
  endif(COMMAND cmake_policy)
  
project(blah2)
include(CMakePrintHelpers)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

SET (PROJECT_ROOT "${PROJECT_SOURCE_DIR}")
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin")
SET (PROJECT_TEST_DIR "${PROJECT_SOURCE_DIR}/test")
SET (PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
SET (PROJECT_BINARY_DIR "${PROJECT_ROOT}/bin")
SET (PROJECT_BINARY_TEST_DIR "${PROJECT_ROOT}/bin/test")
SET (PROJECT_BINARY_TEST_UNIT_DIR "${PROJECT_ROOT}/bin/test/unit")
SET (PROJECT_LIB_DIR "${PROJECT_ROOT}/lib")
MESSAGE ("Source path: ${PROJECT_SOURCE_DIR}")
MESSAGE ("Binary path: ${PROJECT_BINARY_DIR}")
MESSAGE ("Binary test path: ${PROJECT_BINARY_TEST_DIR}")
MESSAGE ("Lib path: ${PROJECT_LIB_DIR}")
MESSAGE ("CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")

find_package(Threads REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(ryml CONFIG REQUIRED)
find_package(httplib REQUIRED)
find_package(Armadillo REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

# find_package(fftw3 REQUIRED)

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

include_directories("${PROJECT_SOURCE_DIR}/capture/")
include_directories("${PROJECT_SOURCE_DIR}/capture/rspduo/")
include_directories("${PROJECT_SOURCE_DIR}/process/ambiguity/")
include_directories("${PROJECT_SOURCE_DIR}/process/clutter/")
include_directories("${PROJECT_SOURCE_DIR}/process/detection/")
include_directories("${PROJECT_SOURCE_DIR}/process/tracker/")
include_directories("${PROJECT_SOURCE_DIR}/process/spectrum/")
include_directories("${PROJECT_SOURCE_DIR}/process/meta/")
include_directories("${PROJECT_SOURCE_DIR}/data/")
include_directories("${PROJECT_SOURCE_DIR}/data/meta/")

# unit tests
add_executable(testAmbiguity
  ${PROJECT_TEST_DIR}/unit/process/ambiguity/TestAmbiguity.cpp
  ${PROJECT_SOURCE_DIR}/data/IqData.cpp
  ${PROJECT_SOURCE_DIR}/data/Map.cpp
  ${PROJECT_SOURCE_DIR}/process/ambiguity/Ambiguity.cpp
  ${PROJECT_SOURCE_DIR}/process/meta/HammingNumber.cpp)
target_link_libraries(testAmbiguity PRIVATE Catch2::Catch2WithMain fftw3 fftw3_threads)
set_target_properties(testAmbiguity PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_TEST_UNIT_DIR}")

add_executable(testTracker
  ${PROJECT_TEST_DIR}/unit/process/tracker/TestTracker.cpp
  ${PROJECT_SOURCE_DIR}/data/Detection.cpp
  ${PROJECT_SOURCE_DIR}/data/Track.cpp
  ${PROJECT_SOURCE_DIR}/process/tracker/Tracker.cpp)
target_link_libraries(testTracker PRIVATE Catch2::Catch2WithMain)
set_target_properties(testTracker PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_TEST_UNIT_DIR}")