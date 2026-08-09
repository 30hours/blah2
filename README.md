# blah2

A real-time radar which can support various SDR platforms. See a live instance at [http://radar4.30hours.dev](http://radar4.30hours.dev).

![blah2 example display](./example.png "blah2")

## Features

- 2 channel processing for a dedicated reference and surveillance signal.
- Multi-channel KrakenSDR processing through KrakenSDR Suite V2 Heimdall output.
- Optional coherent array-reference synthesis and noncoherent surveillance-map fusion.
- Designed to be used with external RF source (for passive radar or active radar).
- Outputs delay-Doppler maps to a web front-end.
- Record raw IQ data by pressing spacebar on the web front-end.
- Saves delay-Doppler maps in a JSON array.

## SDR Support

- [SDRplay RSPDuo](https://www.sdrplay.com/rspduo/).
- [USRP](https://www.ettus.com/products/) (only tested on the B210).
- 2x [HackRF](https://greatscottgadgets.com/hackrf/) with clock synchronisation and hardware trigger.
- 2x [RTL-SDR](https://www.rtl-sdr.com/) with clock synchronisation.
- [KrakenSDR](https://www.krakenrf.com/) with two to eight coherent channels.

### KrakenSDR Suite V2

The Kraken path consumes the standard TCP data stream from
[KrakenSDR Suite V2](https://github.com/krakenrf/krakensdr_suite). It does not
require a patched DAQ. Run `heimdall_v2`, configure frequency and gain in the
Suite, and use `config/config-kraken.yml` as a starting point. BLAH2 accepts
only calibrated antenna frames; calibration/noise and retuning frames are
discarded.

The Kraken-specific fields are:

```yaml
capture:
  fs: 2400000
  fc: 204640000
  device:
    type: "Kraken"
    heimdall:
      host: "127.0.0.1"
      port: 8091
    channel_count: 5
    reference_channel: 0
    surveillance_channels: [0, 1, 2, 3, 4]
process:
  performance:
    surveillance_workers: 0
    fft_threads: 1
  reference_synthesis:
    mode: "array_eigenbeam"
    channels: [0, 1, 2, 3, 4]
```

Suite V2 currently streams at 2.4 MS/s, which must match `capture.fs`. The
Suite's packet metadata is used to verify the channel count and center
frequency; `channel_count` determines how many channels BLAH2 consumes, while
frequency and gain remain configured in the Suite. With `mode: dedicated`,
`reference_channel` is excluded from
`surveillance_channels`, preserving conventional reference/surveillance
operation. With `mode: array_eigenbeam`, the channels listed under
`reference_synthesis.channels` estimate
the dominant common arrival and may also produce independent surveillance
maps. Their ambiguity-map magnitudes are RMS-fused into the ordinary BLAH2 map,
so the API and web UI remain compatible.

`surveillance_channels` defaults to every non-reference channel in dedicated
mode and every capture channel in array-reference mode. The optional
`reference_synthesis.channels` list limits which channels contribute to the
synthesized reference; it defaults to every capture channel.

Two through eight Suite V2 channels are accepted at runtime. Independent
surveillance paths can run concurrently; `surveillance_workers: 0` selects a
CPU-aware automatic limit, while `1` uses serial processing on a constrained
host. `fft_threads` controls the FFTW threads used inside each path. This changes
processing latency only, not map fusion or detector math.

The processor logs each reference update in a machine-readable line containing
the channel count, coherent fraction, coherent gain, and per-channel complex
weight magnitude/phase. This is intended for installation verification; a high
reported gain alone is not proof of improved target detection.

The fused detector output keeps BLAH2's existing timestamp, delay, Doppler and
SNR schema. A Kraken installation therefore remains usable as one ordinary
3lips node. Set the receiver and transmitter coordinates under `location` for
the selected illuminator; the array channels are not separate 3lips
nodes.

The existing `docker/Dockerfile-kraken` builds this same BLAH2 processor with
only the Kraken capture backend dependencies. The default `Dockerfile` remains
the multi-backend build for RSPduo, USRP, HackRF and Kraken installations.

## Services

The build environment consists of a docker-compose.yml file running the following services;

- The radar processor responsible for IQ capture and processing.
- The API middleware responsible for reading TCP ports for delay-Doppler map data, and exposing this on a REST API.
- The web front-end displaying processed radar data.

## Usage

Building the code using the following instructions; 

- Install docker and docker-compose on the host machine.
- Clone this repository to some directory.
- Install SDRplay API to run service on host.
- Edit the `config/config.yml` for desired processing parameters.
- Run the docker-compose command.

```bash
sudo git clone http://github.com/30hours/blah2 /opt/blah2
cd /opt/blah2
sudo chown -R $USER .
sudo chmod a+x ./lib/sdrplay-3.15.2/SDRplay_RSP_API-Linux-3.15.2.run
sudo ./lib/sdrplay-3.15.2/SDRplay_RSP_API-Linux-3.15.2.run --tar -xvf -C ./lib/sdrplay-3.15.2
cd lib/sdrplay-3.15.2/ && sudo ./install_lib.sh && cd ../../
sudo docker network create blah2
sudo systemctl enable docker
sudo docker compose up -d --build
```

Alternatively avoid building and use the pre-built Docker packages;

```bash
sudo docker pull ghcr.io/30hours/blah2:latest
vim docker-compose.yml
--- build: .
+++ image: ghcr.io/30hours/blah2:latest
sudo docker compose up -d
```

The radar processing output is available on [http://localhost:49152](http://localhost:49152).

## Documentation

- See `doxygen` pages hosted at [http://doc.30hours.dev/blah2](http://doc.30hours.dev/blah2).

## Future Work

- Add a tracker in delay-Doppler space.
- Support for the HackRF/RTL-SDR using a front-end mixer, to sample 2 RF channels in 1 stream.
- Add [SoapySDR](https://github.com/pothosware/SoapySDR) support for the [C++ API](https://github.com/pothosware/SoapySDR/wiki/Cpp_API_Example) to include a wide range of SDR platforms.

## FAQ

- If the SDRplay RSPduo does not capture data, restart the API service (on the host) using the script `sudo ./script/blah2_rspduo_restart.bash`.

## Contributing

Pull requests are welcome - especially for adding support for a new SDR. 

- Currently have an issue where the USRP B210 is timing out after 5-10 mins and crashes the code. Convinced it's an issue with my usage of the API - contact me for more info.

## Links

- Join the [Discord](https://discord.gg/ewNQbeK5Zn) chat for sharing results and support.

- Watch a [Youtube video](https://www.youtube.com/watch?v=FF2n28qoTQM) showing the hardware and software setup.

## License

[MIT](https://choosealicense.com/licenses/mit/)
