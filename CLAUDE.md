# CLAUDE.md - System Information for Future Claude Instances

## System Overview
- **Host**: owl@192.168.8.105 (password: radar1)
- **OS**: Debian GNU/Linux on Raspberry Pi 5 (ARM64)
- **Location**: /opt/blah2
- **Project**: blah2-rp5 - Real-time radar system with SDR support

## Project Description
This is a fork of blah2 radar system, modified specifically for Raspberry Pi 5. It's a real-time radar that supports various SDR platforms and outputs delay-Doppler maps to a web front-end.

## Key Features
- 2 channel processing (reference and surveillance signals)
- Web-based front-end for radar visualization
- Support for multiple SDR platforms: SDRplay RSPDuo, USRP B210, HackRF, RTL-SDR, KrakenSDR
- Docker-based deployment with docker-compose
- Real-time IQ data recording (spacebar on web interface)

## Directory Structure
- `api/` - Node.js API middleware (server.js, package.json)
- `config/` - Configuration files (config.yml, device-specific configs)
- `src/` - C++ source code for radar processing
- `html/` - Web front-end files
- `lib/` - External libraries (SDRplay API)
- `docker/` - Docker configuration files
- `test/` - Test data and scripts
- `save/` - Directory for saving processed data

## Services (Docker Compose)
1. **blah2** - Main radar processor (C++ application)
2. **blah2_web** - Apache web server (port 49152)
3. **blah2_api** - Node.js API middleware
4. **blah2_host** - Reverse proxy for frontend

## Common Operations

### Starting the System
```bash
cd /opt/blah2
sudo docker compose up -d --build
```

### Stopping the System
```bash
cd /opt/blah2
sudo docker compose down
```

### Checking Status
```bash
sudo docker ps
sudo docker logs blah2
sudo docker logs blah2-api
```

### SDRplay Restart (if needed)
```bash
sudo ./sdrplay-restart.sh
```

### Configuration
- Main config: `config/config.yml`
- Device-specific configs available in `config/` directory

## Network Access
- Web interface: http://localhost:49152
- Uses Docker network named "blah2"
- Host networking mode for SDR access

## Important Notes
- Runs with privileged Docker containers for hardware access
- Requires SDRplay API installation on host
- Uses /dev/shm for shared memory operations
- USB devices mounted into containers
- System designed for real-time radar processing

## Troubleshooting
- If SDRplay RSPduo doesn't capture data, restart API service
- Check Docker logs for debugging
- Ensure SDR hardware is properly connected
- Verify configuration files are correct

## File Ownership
- Owner: owl (user)
- Some files owned by root (installation scripts)
- Permissions set appropriately for Docker access

## Build System
- CMake-based build system
- Dockerfile for containerization
- Uses external libraries for SDR support

Last Updated: $(date)
