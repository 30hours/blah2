# Components 

- Raspberry Pi 5 (RPI) with 16GB RAM.
- 2x HackRF One SDR.
- (Optional, but highly recommended) HackRF TCXO.
- 2x antennas tuned for the chosen frequency.
- (If using ADS-B) Omni-directional ADS-B antenna.
- Good quality RF cables. Ideally, cables with 50ohm impedance.
- RF Cable connectors (e.g. SMA).
- (If using ADS-B) RTL-sdr (RTL2832U & R820T2 are fine. No need for an expensive one).
- (If using ADS-B) Low Noise Amplifier (LNA).
- (If using ADS-B or a cooling fan) DC-to-DC Converter.
- (Optional) Cooling fan (5v or 12v).
- USB Cables (to connect HackRf One and RTL-sdr to RPI).
- Soldering kit.
- Cable crimping tool.
- Heatshrink sleeves.
- Patch cables (to power LNA, use RPI GPIO, etc.).

# Physical build

**Circuit Diagram**

<img width="898" height="484" alt="HackRF-Circuit-diagram" src="https://github.com/user-attachments/assets/f7eaefdf-f8d7-41a9-b56f-dfaa40f566fa" />


**Wiring of the 2 HackRF One devices**

<img width="1100" height="1058" alt="HackRF-wiring" src="https://github.com/user-attachments/assets/313615b9-5ef6-4165-a537-a4924f94ec22" />

- If you are using a TCXO to enhance the clock stability, put it on the Surveillance device as shown below.

<img width="994" height="542" src="https://github.com/user-attachments/assets/d157d47c-bcb3-4411-9916-32b2aa0c02ed" />

  
- If you are using an original HackRF One, make sure to have an antenna plugged in all the time when operating the SDR. Otherwise, you may damage the board. (The Clifford Heath version which you can find on AliExpress has a protection circuit against this).
- Some of the ADS-B packages you'll find below may fail to install correctly if you don't have the RTL-sdr dongle plugged in.

# Software Installation

- Install Ubuntu Server 25.10
- Update Ubuntu
- Upgrade Ubuntu
- Install Docker by [using the apt repository](https://docs.docker.com/engine/install/ubuntu/)
- Install HackRF firmware and tools
  ```bash
  sudo apt install hackrf
  ```
- Install RTL-sdr tools
  ```bash
  sudo apt install rtl-sdr*
  ```
- Install [blah2](https://github.com/30hours/blah2)
- If the installation had failed, this is likely due to the default Dockerfile. It was written for x64 machines. You need to replace it with [this Dockerfile](/30hours/blah2/blob/main/contrib/raspberrypi/Dockerfile) that was written for ARM devices.
- Obtain the HackRF One serial numbers by running ```hackrf_info```.
- Make sure to place the correct config file for HackRF One in ```/opt/blah2/config/config.yml```. Another type of SDR device config could be there be default. All example config files reside in the same directory.
- At this stage, you just need to put the serial numbers of the reference and surveillance HackRF One devices. Tuning other parameters will be discussed separately.
- (Optional) As I have been testing outdoors, it may take the HackRF board a bit of time after a cold start to warm up and stabilise. Hence, I force blah2 to restart after 10mins of boot time
  ```bash
  sudo crontab -e
  ```
  Then, add the following line to the end of the file
  ```bash
  @reboot sleep 600 && sudo docker compose -f /opt/blah2/docker-compose.yml down; sudo docker compose -f /opt/blah2/docker-compose.yml up
  ```
- Install [adsb2dd](https://github.com/30hours/adsb2dd). No additional configuration is required for adsb2dd.
- Install readsb (it helps if the rtl-sdr dongle is plugged in!). Instruction can be found [here](https://github.com/wiedehopf/adsb-scripts/wiki/Automatic-installation-for-readsb).
- Set your radar location by putting the right coordinates. (latitude, longitude)
  ```bash
  sudo readsb-set-location 0.00000001 0.00000001
  ```
- Install [tar1090](https://github.com/wiedehopf/tar1090). After installation, do the following.
```bash
sudo mv /etc/lighttpd/conf-enabled/88-tar1090.conf /etc/lighttpd/conf-enabled/88-tar1090.conf_archived
```
The service can be accessed now from ```http://ip_address:8504/```

**Cooling fan - Optional**

- If you're going to install a cooling fan in your enclosure to cool it down, install "sensors" to get readings from sensors onboard the RPI.
```bash
sudo apt install lm-sensors
sudo sensors-detect
sensors
```
- Create the file ```create /opt/automation/fan.sh``` and copy the content of [fan.sh](./fan.sh) inside it.
- Run
```bash
chmod 755 /opt/automation/fan.sh
sudo crontab -e
```
Insert the following line at the end of the file
```bash
* * * * * sudo /opt/automation/fan.sh
```

# Tuning and Troubleshooting

TBC


