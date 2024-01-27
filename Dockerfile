FROM ubuntu:22.04 as blah2_env
LABEL maintainer="30hours <nathan@30hours.dev>"

WORKDIR /blah2
ADD lib lib
RUN apt-get update \
  && apt-get install -y g++ make cmake git curl zip unzip doxygen graphviz \
  libfftw3-dev pkg-config gfortran \
    # UHD api dependencies
  autoconf automake build-essential ccache cpufrequtils ethtool inetutils-tools \
  libboost-all-dev libncurses5 libncurses5-dev libusb-1.0-0 libusb-1.0-0-dev libusb-dev python3-dev \
  python3-mako python3-numpy python3-requests python3-scipy python3-setuptools python3-ruamel.yaml \
  && apt-get autoremove -y \
  && apt-get clean -y \
  && rm -rf /var/lib/apt/lists/*

# install dependencies from vcpkg
RUN git clone https://github.com/microsoft/vcpkg /opt/vcpkg \
  && /opt/vcpkg/bootstrap-vcpkg.sh
ENV PATH="/opt/vcpkg:${PATH}" VCPKG_ROOT=/opt/vcpkg
RUN cd /blah2/lib && vcpkg integrate install \
  && vcpkg install --clean-after-build

# install SDRplay API
RUN chmod +x /blah2/lib/sdrplay-3.0.7/SDRplay_RSP_API-Linux-3.07.1.run \ 
  && /blah2/lib/sdrplay-3.0.7/SDRplay_RSP_API-Linux-3.07.1.run --tar -xvf -C /blah2/lib/sdrplay-3.0.7 \ 
  && cp /blah2/lib/sdrplay-3.0.7/x86_64/libsdrplay_api.so.3.07  /usr/local/lib/libsdrplay_api.so \ 
  && cp /blah2/lib/sdrplay-3.0.7/x86_64/libsdrplay_api.so.3.07 /usr/local/lib/libsdrplay_api.so.3.07 \ 
  && cp /blah2/lib/sdrplay-3.0.7/inc/* /usr/local/include \ 
  && chmod 644 /usr/local/lib/libsdrplay_api.so /usr/local/lib/libsdrplay_api.so.3.07 \ 
  && ldconfig

# install UHD API
RUN git clone https://github.com/MicroPhase/antsdr_uhd /opt/uhd \
  && mkdir -p /opt/uhd/host/build && cd /opt/uhd/host/build \
  && cmake -DCMAKE_INSTALL_PREFIX=/opt/uhd -DENABLE_PYTHON_API=OFF \
  -DENABLE_EXAMPLES=OFF -DENABLE_TESTS=OFF -DENABLE_X400=OFF \
  -DENABLE_N320=OFF -DENABLE_X300=OFF -DENABLE_USRP2=OFF -DENABLE_USRP1=OFF \
  -DENABLE_N300=OFF -DENABLE_E320=OFF -DENABLE_E300=OFF \
  -DENABLE_MANUAL=OFF -DENABLE_DOXYGEN=OFF -DENABLE_MAN_PAGES=OFF \
  -DENABLE_DPDK=OFF ../ && make && make install && ldconfig
ENV LD_LIBRARY_PATH=/opt/uhd/lib:$LD_LIBRARY_PATH
RUN /opt/uhd/lib/uhd/utils/uhd_images_downloader.py

FROM blah2_env as blah2
LABEL maintainer="30hours <nathan@30hours.dev>"

ADD src src
ADD test test
ADD CMakeLists.txt CMakePresets.json Doxyfile /blah2/
RUN ls -lah /opt/uhd/lib/cmake/uhd/ && cat /opt/uhd/lib/cmake/uhd/UHDConfigVersion.cmake
RUN mkdir -p build && cd build && cmake -S . --preset prod-release \
  -DCMAKE_PREFIX_PATH=/blah2/lib/vcpkg_installed/x64-linux/share .. \
  && cd prod-release && make
RUN chmod +x bin/blah2
