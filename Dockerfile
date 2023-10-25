FROM ubuntu:18.04 as blah2_env
MAINTAINER 30hours <nathan@30hours.dev>

WORKDIR blah2
ADD lib lib
RUN ls -lah && pwd
RUN apt-get update
RUN apt-get install -y g++ make cmake libfftw3-dev liblapack-dev libopenblas-dev xz-utils libudev-dev libusb-1.0.0-dev sudo systemd doxygen graphviz
RUN cd lib && tar xf armadillo-12.0.1.tar.xz && cd armadillo-12.0.1 && cmake . && make install

RUN chmod +x /blah2/lib/sdrplay-3.0.7/SDRplay_RSP_API-Linux-3.07.1.run \ 
&& /blah2/lib/sdrplay-3.0.7/SDRplay_RSP_API-Linux-3.07.1.run --tar -xvf -C /blah2/lib/sdrplay-3.0.7 \ 
&& cp /blah2/lib/sdrplay-3.0.7/x86_64/libsdrplay_api.so.3.07  /usr/local/lib/libsdrplay_api.so \ 
&& cp /blah2/lib/sdrplay-3.0.7/x86_64/libsdrplay_api.so.3.07 /usr/local/lib/libsdrplay_api.so.3.07 \ 
&& cp /blah2/lib/sdrplay-3.0.7/inc/* /usr/local/include \ 
&& chmod 644 /usr/local/lib/libsdrplay_api.so /usr/local/lib/libsdrplay_api.so.3.07 \ 
&& ldconfig

FROM blah2_env as blah2
MAINTAINER 30hours <nathan@30hours.dev>

ADD . .
RUN ls -lah /usr/local/include
RUN rm -rf build && mkdir -p build && cd build && cmake .. && make
RUN chmod +x bin/blah2
