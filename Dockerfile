FROM ubuntu:22.04 as blah2_env

LABEL maintainer="30hours <nathan@30hours.dev>"
LABEL org.opencontainers.image.source https://github.com/30hours/blah2

ARG UHD_TAG=v4.6.0.0
ARG MAKEWIDTH=8

# Install UHD from source, modified from https://github.com/EttusResearch/ettus-docker/blob/master/ubuntu-uhd/Dockerfile
RUN apt-get update && \
    apt-get -y install -q \
        build-essential \
        ccache \
        cmake \
        curl \
        doxygen \
        dpdk \
        git \
        libboost-all-dev \
        libdpdk-dev \
        libudev-dev \
        libusb-1.0-0-dev \
        python3-dev \
        python3-docutils \
        python3-mako \
        python3-numpy \
        python3-pip \
        python3-requests && \
    rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/EttusResearch/uhd.git /uhd && \
    cd /uhd/ && git checkout $UHD_TAG && \
    mkdir -p /uhd/host/build

WORKDIR /uhd/host/build

RUN cmake .. -DENABLE_PYTHON3=ON -DUHD_RELEASE_MODE=release -DCMAKE_INSTALL_PREFIX=/usr && \
    make -j $MAKEWIDTH && \
    make test && \
    make install

RUN uhd_images_downloader

WORKDIR /blah2

ADD lib lib

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get install -y \
        g++ \
        curl \
        zip \
        unzip \
        doxygen \
        graphviz \
        libfftw3-dev \
        pkg-config \
        gfortran \
        ninja-build && \
    apt-get autoremove -y && \
    apt-get clean -y && \
    rm -rf /var/lib/apt/lists/*

# Install dependencies from vcpkg
ENV PATH="/opt/vcpkg:${PATH}" VCPKG_ROOT=/opt/vcpkg

# Detect architecture and set environment variable accordingly
RUN if [ "$(uname -m)" = "aarch64" ]; then \
        export VCPKG_FORCE_SYSTEM_BINARIES=arm; \
    else \
        export VCPKG_FORCE_SYSTEM_BINARIES=; \
    fi && \
    git clone https://github.com/microsoft/vcpkg /opt/vcpkg && \
    /opt/vcpkg/bootstrap-vcpkg.sh && \
    cd /blah2/lib && vcpkg integrate install && \
    vcpkg install --clean-after-build

# Install SDRplay API based on architecture
RUN chmod +x /blah2/lib/sdrplay-3.14.0/SDRplay_RSP_API-Linux-3.14.0.run && \
    /blah2/lib/sdrplay-3.14.0/SDRplay_RSP_API-Linux-3.14.0.run --tar -xvf -C /blah2/lib/sdrplay-3.14.0 && \
    ARCH=$(uname -m) && \
    cp /blah2/lib/sdrplay-3.14.0/${ARCH}/libsdrplay_api.so.3.14 /usr/local/lib/libsdrplay_api.so && \
    cp /blah2/lib/sdrplay-3.14.0/${ARCH}/libsdrplay_api.so.3.14 /usr/local/lib/libsdrplay_api.so.3.14 && \
    cp /blah2/lib/sdrplay-3.14.0/inc/* /usr/local/include && \
    chmod 644 /usr/local/lib/libsdrplay_api.so /usr/local/lib/libsdrplay_api.so.3.14 && \
    ldconfig

FROM blah2_env as blah2

LABEL maintainer="30hours <nathan@30hours.dev>"

ADD src src
ADD test test
ADD CMakeLists.txt CMakePresets.json Doxyfile /blah2/

RUN if [ "$(uname -m)" = "aarch64" ]; then \
        export VCPKG_TARGET_TRIPLET=arm64-linux; \
    elif [ "$(uname -m)" = "x86_64" ]; then \
        export VCPKG_TARGET_TRIPLET=x64-linux; \
    else \
        echo "Unsupported architecture: $(uname -m)"; exit 1; \
    fi && \
    mkdir -p build && cd build && cmake -S . --preset prod-release \
    -DCMAKE_PREFIX_PATH=/blah2/lib/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share .. && \
    cd prod-release && make

RUN chmod +x bin/blah2