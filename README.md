# blah2

A real-time radar which can support various SDR platforms.

![blah2 example display](./example.png "blah2")

## Features

- Currently only support for the [SDRplay RSPDuo](https://www.sdrplay.com/rspduo/).
- 2 channel processing for a reference and surveillance signal.
- Designed as a passive radar, but can also work as an active radar.
- Outputs delay-Doppler maps to a web front-end.
- Record raw IQ data by pressing spacebar on the web front-end.
- Saves delay-Doppler maps in a *json* array.

## Services

The build environment consists of a docker-compose.yml file running the following services;

- The radar processor responsible for IQ capture and processing.
- The API middleware responsible for reading TCP ports for delay-Doppler map data, and exposing this on a REST API.
- The web front-end displaying processed radar data.

## Usage

- Install docker and docker-compose on the host machine.
- Clone this repository to some directory.
- Install dependencies from Dockerfile.
- Edit the config.yml for desired processing parameters.
- Run the docker-compose command.

```bash
git clone http://github.com/30hours/blah2
cd blah2
vim config/config.yml
sudo mkdir /opt/blah2
sudo chmod a+rw /opt/blah2

sudo docker-compose up -d blah2_frontend blah2_api
mkdir build && cd build
cmake .. && make && cd ..
./bin/blah2 -c config/config.yml
```

The radar processing output is available on [http://localhost:49152](http://localhost:49152).

## Documentation

- See `doxygen` pages hosted at [http://doc.30hours.dev/blah2](http://doc.30hours.dev/blah2).

## Future Work

- The blah2 service can be built in Docker, except for the SDRplay API due to its dependence on *systemd*. This service needs to be built manually with *cmake* at present.
- A CFAR detector has not yet been implemented.
- Support for the HackRF and RTL-SDR using front-end mixer to sample 2 RF channels in 1 stream.
- Occasional segmentation fault from a mutex issue.

## FAQ

- If the SDRplay RSPduo does not capture data, restart the API service using `sudo systemctl restart sdrplay.api`.

## Contributing

Pull requests are welcome - especially for adding support for a new SDR.

## License

[MIT](https://choosealicense.com/licenses/mit/)