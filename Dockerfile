FROM debian:bookworm as blah2_env
LABEL maintainer="Jehan <jehan.azad@gmail.com>"
LABEL org.opencontainers.image.source https://github.com/30hours/blah2

WORKDIR /opt/blah2
ADD lib lib

# Install base dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    cmake \
    ninja-build \
    git \
    curl \
    zip \
    unzip \
    doxygen \
    graphviz \
    expect \
    librtlsdr-dev \
    libfftw3-dev \
    pkg-config \
    gfortran \
    libhackrf-dev \
    libusb-dev \
    libusb-1.0.0-dev \
    python3-pip \
    python3-mako \
    python3-numpy \
    libboost-all-dev \
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

# Add this new section to build UHD from source
RUN git clone https://github.com/EttusResearch/uhd.git /opt/uhd && \
    cd /opt/uhd && \
    git checkout v4.6.0.0 && \
    cd host && \
    mkdir build && \
    cd build && \
    cmake .. \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# install dependencies from vcpkg
ENV VCPKG_ROOT=/opt/vcpkg
ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++
RUN export PATH="/opt/vcpkg:${PATH}" \
    && git clone https://github.com/microsoft/vcpkg /opt/vcpkg \
    && if [ "$(uname -m)" = "aarch64" ]; then export VCPKG_FORCE_SYSTEM_BINARIES=1; fi \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics \
    && cd /opt/blah2/lib && vcpkg integrate install \
    && vcpkg install --clean-after-build

# install SDRplay API
USER root

RUN export ARCH=$(uname -m) \
    && if [ "$ARCH" = "x86_64" ]; then \
         ARCH="amd64"; \
    elif [ "$ARCH" = "aarch64" ]; then \
         ARCH="arm64"; \
    fi \
    && export MAJVER="3.15" \
    && export MINVER="2" \
    && export VER=${MAJVER}.${MINVER} \
    && cd /opt/blah2/lib/sdrplay-${VER} \
    && chmod +x SDRplay_RSP_API-Linux-${VER}.run \
    && ./SDRplay_RSP_API-Linux-${VER}.run --noexec --target /opt/blah2/lib/sdrplay-${VER}/extract \


    # Then manually copy the files to the target location
    && cp -r /opt/blah2/lib/sdrplay-${VER}/extract/* /opt/blah2/lib/sdrplay-${VER}/ \

    && cp ${ARCH}/libsdrplay_api.so.${MAJVER} /usr/local/lib/libsdrplay_api.so.${MAJVER} \
    && cp inc/* /usr/local/include \
    && chmod 644 /usr/local/lib/libsdrplay_api.so.${MAJVER} 

# install UHD API
RUN uhd_images_downloader

# install RTL-SDR API
RUN git clone https://github.com/krakenrf/librtlsdr /opt/librtlsdr \
    && cd /opt/librtlsdr && mkdir build && cd build \
    && cmake ../ -DINSTALL_UDEV_RULES=ON -DDETACH_KERNEL_DRIVER=ON && make && make install && ldconfig


FROM blah2_env as blah2
LABEL maintainer="30hours <nathan@30hours.dev>"

WORKDIR /opt/blah2

ADD src src
ADD test test
ADD CMakeLists.txt CMakePresets.json Doxyfile ./


# Updated build step to use the correct binary location
RUN set -ex \
    && mkdir -p build \
    && cd build \
    && cmake -S .. --preset prod-release \
        -DCMAKE_PREFIX_PATH=$(echo /opt/blah2/lib/vcpkg_installed/*/share) .. \
    && cd prod-release \
    && make \
    && echo "==== Binary location: ====" \
    && ls -l /opt/blah2/bin/blah2 \
    && mkdir -p /blah2/bin \
    && cp -v /opt/blah2/bin/blah2 /blah2/bin/ \
    && chmod +x /blah2/bin/blah2

WORKDIR /blah2/bin
