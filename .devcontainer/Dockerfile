# TODO: update devcontainer with vcpkg manifest

# ubuntu-22.04 by default
ARG VARIANT="jammy"
FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

ENV DEBIAN_FRONTEND=noninteractive
# Feel like this shouldn't be needed but it drops me in / during build
WORKDIR /workspace

RUN apt-get update \
    #
    # Install dev tools and package dependencies 
    && apt-get install -y clang-tidy clang-format doxygen graphviz gfortran \
    libfftw3-dev liblapack-dev libopenblas-dev libudev-dev libusb-1.0.0-dev \
    #
    # Clean up
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*
    
# Install dependencies from vcpkg
RUN vcpkg integrate install \
    && vcpkg install catch2 \
    && vcpkg install rapidjson \
    && vcpkg install asio \
    && vcpkg install cpp-httplib \
    && vcpkg install armadillo \
    && vcpkg install ryml
    
COPY lib/sdrplay-3.0.7/SDRplay_RSP_API-Linux-3.07.1.run /workspace/

# Install shitty sdrplay API
RUN chmod +x /workspace/SDRplay_RSP_API-Linux-3.07.1.run \
    && /workspace/SDRplay_RSP_API-Linux-3.07.1.run --tar -xf \
    && cp x86_64/libsdrplay_api.so.3.07  /usr/local/lib/libsdrplay_api.so \ 
    && ln -s /usr/local/lib/libsdrplay_api.so /usr/local/lib/libsdrplay_api.so.3.07 \ 
    && cp inc/* /usr/local/include \ 
    && chmod 644 /usr/local/lib/libsdrplay_api.so /usr/local/lib/libsdrplay_api.so.3.07 \
    && ldconfig
    
