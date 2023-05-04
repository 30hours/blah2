FROM ubuntu:18.04 as blah2_env
MAINTAINER 30hours <nathan@30hours.dev>

WORKDIR blah2
ADD lib lib
RUN apt-get update
RUN apt-get install -y g++ make cmake libfftw3-dev liblapack-dev libopenblas-dev xz-utils libudev-dev libusb-1.0.0-dev sudo systemd
RUN cd lib && tar xf armadillo-12.0.1.tar.xz && cd armadillo-12.0.1 && cmake . && make install
RUN cd lib/sdrplay-3.0.7 && mkdir -p /etc/udev/rules.d && yes | ./install_lib.sh

FROM blah2_env as blah2
MAINTAINER 30hours <nathan@30hours.dev>

ADD . .
RUN rm -rf build && mkdir -p build && cd build && cmake .. && make
RUN chmod +x bin/blah2
