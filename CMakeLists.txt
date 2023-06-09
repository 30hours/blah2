project(blah2)
cmake_minimum_required(VERSION 3.8)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

SET (PROJECT_ROOT "${PROJECT_SOURCE_DIR}")
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin")
SET (PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
SET (PROJECT_BINARY_DIR "${PROJECT_ROOT}/bin")
SET (PROJECT_LIB_DIR "${PROJECT_ROOT}/lib")
MESSAGE ("Source path: ${PROJECT_SOURCE_DIR}")
MESSAGE ("Binary path: ${PROJECT_BINARY_DIR}")
MESSAGE ("Lib path: ${PROJECT_LIB_DIR}")

add_executable(blah2 
  ${PROJECT_SOURCE_DIR}/blah2.cpp
  ${PROJECT_SOURCE_DIR}/capture/Capture.cpp
  ${PROJECT_SOURCE_DIR}/capture/rspduo/RspDuo.cpp
  ${PROJECT_SOURCE_DIR}/process/ambiguity/Ambiguity.cpp
  ${PROJECT_SOURCE_DIR}/process/clutter/WienerHopf.cpp
  ${PROJECT_SOURCE_DIR}/data/IqData.cpp
  ${PROJECT_SOURCE_DIR}/data/Map.cpp
)

add_library(ryml ${PROJECT_LIB_DIR}/rapidyaml-0.5.0/ryml-0.5.0.hpp)
add_library(rapidjson ${PROJECT_LIB_DIR}/rapidjson-1.1.0/)
add_library(sdrplay /usr/local/include/sdrplay_api.h)
add_library(asio ${PROJECT_LIB_DIR}/asio-1.26.0/asio.hpp)
add_library(httplib ${PROJECT_LIB_DIR}/cpp-httplib-0.12.2/httplib.h)

include_directories("${PROJECT_LIB_DIR}/rapidjson-1.1.0/")
set_target_properties(rapidjson PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(blah2 rapidjson)

include_directories("${PROJECT_LIB_DIR}/asio-1.26.0/")
set_target_properties(asio PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(blah2 asio)

include_directories("${PROJECT_LIB_DIR}/cpp-httplib-0.12.2/")
set_target_properties(httplib PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(blah2 httplib)

include_directories("${PROJECT_LIB_DIR}/rapidyaml-0.5.0/")
set_target_properties(ryml PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(blah2 ryml)

set_target_properties(sdrplay PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(sdrplay /usr/local/lib/libsdrplay_api.so.3.07)
target_link_libraries(blah2 sdrplay)

target_link_libraries(blah2 fftw3)
target_link_libraries(blah2 fftw3_threads)
target_link_libraries(blah2 lapack)
target_link_libraries(blah2 blas)
target_link_libraries(blah2 armadillo)

include_directories("${PROJECT_SOURCE_DIR}/capture/")
include_directories("${PROJECT_SOURCE_DIR}/capture/rspduo/")
include_directories("${PROJECT_SOURCE_DIR}/process/ambiguity/")
include_directories("${PROJECT_SOURCE_DIR}/process/clutter/")
include_directories("${PROJECT_SOURCE_DIR}/data/")
