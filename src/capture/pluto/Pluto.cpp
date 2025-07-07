#include "capture/pluto/Pluto.h"

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <complex>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <vector>
#include <algorithm>

// Constructor
Pluto::Pluto(std::string _type, uint32_t _fc, uint32_t _fs, 
             std::string _path, bool *_saveIq,
             std::string _uri, std::string _gain_mode, int _gain_rx,
             std::string _rf_port, uint32_t _bandwidth)
    : Source(_type, _fc, _fs, _path, _saveIq),
      uri(_uri),
      ctx(nullptr),
      phy(nullptr),
      rx(nullptr),
      rx0_i(nullptr),
      rx0_q(nullptr),
      rx1_i(nullptr),
      rx1_q(nullptr),
      rxbuf(nullptr),
      gain_mode(_gain_mode),
      gain_rx(_gain_rx),
      rf_port(_rf_port),
      bandwidth(_bandwidth),
      buffer_size(1024 * 32),
      running(false)
{
    // Default buffer size already set in initializer list
}

// Destructor
Pluto::~Pluto() {
    stop();
    cleanup();
}

bool Pluto::init_context() {
    std::cout << "[Pluto] Initializing IIO context for device: " << uri << std::endl;
    
    // Create IIO context
    ctx = iio_create_context_from_uri(uri.c_str());
    if (!ctx) {
        std::cerr << "[Pluto] Error: Could not create IIO context for " << uri << std::endl;
        return false;
    }
    
    // Find the AD9361 PHY device
    phy = iio_context_find_device(ctx, "ad9361-phy");
    if (!phy) {
        std::cerr << "[Pluto] Error: Could not find AD9361 PHY device" << std::endl;
        return false;
    }
    
    // Find the RX device
    rx = iio_context_find_device(ctx, "cf-ad9361-lpc");
    if (!rx) {
        std::cerr << "[Pluto] Error: Could not find RX device" << std::endl;
        return false;
    }
    
    std::cout << "[Pluto] IIO context initialized successfully" << std::endl;
    return true;
}

void Pluto::configure_rx_channel(struct iio_channel *channel) {
    if (!channel) return;
    
    // Set gain mode
    iio_channel_attr_write(channel, "gain_control_mode", gain_mode.c_str());
    
    // Set manual gain if in manual mode
    if (gain_mode == "manual") {
        iio_channel_attr_write_double(channel, "hardwaregain", gain_rx);
    }
    
    // Set RF port
    iio_channel_attr_write(channel, "rf_port_select", rf_port.c_str());
    
    // Enable all tracking options
    iio_channel_attr_write_bool(channel, "quadrature_tracking_en", true);
    iio_channel_attr_write_bool(channel, "rf_dc_offset_tracking_en", true);
    iio_channel_attr_write_bool(channel, "bb_dc_offset_tracking_en", true);
}
bool Pluto::configure_device() {
    std::cout << "[Pluto] Configuring PlutoSDR device parameters" << std::endl;
    
    // Set the AD9361 to 2R2T mode
    iio_device_attr_write(phy, "adi,2rx-2tx-mode-enable", "1");
    
    // Set the center frequency
    iio_channel_attr_write_longlong(
        iio_device_find_channel(phy, "altvoltage0", true),  // RX LO
        "frequency",
        static_cast<long long>(fc));
    
    // Set the sample rate
    iio_channel_attr_write_longlong(
        iio_device_find_channel(phy, "voltage0", false),    // RX channel
        "sampling_frequency",
        static_cast<long long>(fs));
    
    // Set the RF bandwidth
    iio_channel_attr_write_longlong(
        iio_device_find_channel(phy, "voltage0", false),    // RX channel
        "rf_bandwidth",
        static_cast<long long>(bandwidth));
    
    // Enable auto-calibration
    iio_device_attr_write(phy, "calib_mode", "auto");
    iio_device_attr_write_bool(iio_device_find_channel(phy, "voltage0", false), "calibrate", true);
    iio_device_attr_write_bool(iio_device_find_channel(phy, "voltage1", false), "calibrate", true);  

  
    // Configure RX channels
    configure_rx_channel(iio_device_find_channel(phy, "voltage0", false)); // RX channel 0
    configure_rx_channel(iio_device_find_channel(phy, "voltage1", false)); // RX channel 1
    
    std::cout << "[Pluto] Device parameters configured successfully" << std::endl;
    return true;
}

bool Pluto::setup_channels() {
    std::cout << "[Pluto] Setting up RX channels" << std::endl;
    
    // Find and enable I/Q channels for RX0
    rx0_i = iio_device_find_channel(rx, "voltage0", false);
    rx0_q = iio_device_find_channel(rx, "voltage1", false);
    
    if (!rx0_i || !rx0_q) {
        std::cerr << "[Pluto] Error: Could not find I/Q channels for RX0" << std::endl;
        return false;
    }
    
    iio_channel_enable(rx0_i);
    iio_channel_enable(rx0_q);
    
    // Find and enable I/Q channels for RX1
    rx1_i = iio_device_find_channel(rx, "voltage2", false);
    rx1_q = iio_device_find_channel(rx, "voltage3", false);
    
    if (!rx1_i || !rx1_q) {
        std::cerr << "[Pluto] Error: Could not find I/Q channels for RX1" << std::endl;
        return false;
    }
    
    iio_channel_enable(rx1_i);
    iio_channel_enable(rx1_q);
    
    std::cout << "[Pluto] RX channels set up successfully" << std::endl;
    return true;
}

bool Pluto::create_buffer() {
    std::cout << "[Pluto] Creating IIO buffer of size " << buffer_size << std::endl;
    
    // Create IIO buffer for RX
    rxbuf = iio_device_create_buffer(rx, buffer_size, false);
    if (!rxbuf) {
        std::cerr << "[Pluto] Error: Could not create IIO buffer" << std::endl;
        return false;
    }
    
    std::cout << "[Pluto] IIO buffer created successfully" << std::endl;
    return true;
}

void Pluto::capture_thread(IqData *buffer1, IqData *buffer2) {
    // Allocate temporary buffer for raw samples
    size_t sample_size = iio_device_get_sample_size(rx);
    std::cout << "[Pluto] Sample size: " << sample_size << " bytes" << std::endl;
    
    // Batch size for buffer locking - process this many samples between locks
    // Adjust this value based on your performance needs
    const size_t BATCH_SIZE = 128;
    
    // Capture loop
    while (running) {
        // Refill the buffer
        ssize_t nbytes = iio_buffer_refill(rxbuf);
        if (nbytes < 0) {
            std::cerr << "[Pluto] Error: Buffer refill failed: " << nbytes << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // Get pointers to the buffer
        void *start = iio_buffer_first(rxbuf, rx0_i);
        size_t step = iio_buffer_step(rxbuf);
        size_t count = (size_t)nbytes / step;
        
        // Process samples in batches to reduce lock contention
        for (size_t batch_start = 0; batch_start < count; batch_start += BATCH_SIZE) {
            // Calculate the end of this batch (or the end of the buffer)
            size_t batch_end = std::min(batch_start + BATCH_SIZE, count);
            
            // Lock buffers for this batch only
            buffer1->lock();
            buffer2->lock();
            
            // Process sample batch
            for (size_t i = batch_start; i < batch_end; i++) {
                // Get sample pointer
                int16_t *sample = (int16_t *)((char *)start + i * step);
                
                // Extract I/Q for both channels
                int16_t i0 = sample[0];  // Channel 0 I
                int16_t q0 = sample[1];  // Channel 0 Q
                int16_t i1 = sample[2];  // Channel 1 I
                int16_t q1 = sample[3];  // Channel 1 Q
                
                // Push to buffers (normalized to match your expected format)
                buffer1->push_back({static_cast<double>(i0), static_cast<double>(q0)});
                buffer2->push_back({static_cast<double>(i1), static_cast<double>(q1)});
                
                // Write to file if saving is enabled
                if (*saveIq && saveIqFile.is_open()) {
                    saveIqFile.write(reinterpret_cast<char*>(&i0), sizeof(int16_t));
                    saveIqFile.write(reinterpret_cast<char*>(&q0), sizeof(int16_t));
                    saveIqFile.write(reinterpret_cast<char*>(&i1), sizeof(int16_t));
                    saveIqFile.write(reinterpret_cast<char*>(&q1), sizeof(int16_t));
                }
            }
            
            // Unlock buffers after batch processing
            buffer1->unlock();
            buffer2->unlock();
            
            // Check if we should exit mid-processing
            if (!running) break;
        }
    }
}

void Pluto::validate() {
    std::cout << "[Pluto] Validating configuration parameters" << std::endl;
    
    // Check frequency range (70MHz to 6GHz for Pluto)
    if (fc < 70000000 || fc > 6000000000) {
        std::cerr << "[Pluto] Error: Frequency must be between 70 MHz and 6 GHz" << std::endl;
        throw std::invalid_argument("Invalid frequency");
    }
    
    // Check sample rate (<=61.44 MHz for Pluto)
    if (fs > 61440000) {
        std::cerr << "[Pluto] Error: Sample rate must be <= 61.44 MHz" << std::endl;
        throw std::invalid_argument("Invalid sample rate");
    }
    
    // Check gain value (0-71 dB for Pluto)
    if (gain_rx < 0 || gain_rx > 71) {
        std::cerr << "[Pluto] Error: RX gain must be between 0 and 71 dB" << std::endl;
        throw std::invalid_argument("Invalid gain value");
    }
    
    // Print configuration
    std::cout << "[Pluto] Configuration parameters:" << std::endl;
    std::cout << "  URI: " << uri << std::endl;
    std::cout << "  Center frequency: " << fc << " Hz" << std::endl;
    std::cout << "  Sample rate: " << fs << " Hz" << std::endl;
    std::cout << "  Bandwidth: " << bandwidth << " Hz" << std::endl;
    std::cout << "  Gain mode: " << gain_mode << std::endl;
    std::cout << "  RX gain: " << gain_rx << " dB" << std::endl;
    std::cout << "  RF port: " << rf_port << std::endl;
    std::cout << "  Buffer size: " << buffer_size << " samples" << std::endl;
}

void Pluto::cleanup() {
    std::cout << "[Pluto] Cleaning up resources" << std::endl;
    
    // Destroy buffer
    if (rxbuf) {
        iio_buffer_destroy(rxbuf);
        rxbuf = nullptr;
    }
    
    // Destroy context
    if (ctx) {
        iio_context_destroy(ctx);
        ctx = nullptr;
    }
    
    std::cout << "[Pluto] Cleanup complete" << std::endl;
}

void Pluto::start() {
    std::cout << "[Pluto] Starting device" << std::endl;
    
    // Initialize and configure the device
    if (!init_context() || !configure_device() || !setup_channels() || !create_buffer()) {
        std::cerr << "[Pluto] Error: Failed to initialize device" << std::endl;
        cleanup();
        throw std::runtime_error("Device initialization failed");
    }
    
    // Validate configuration
    validate();
    
    std::cout << "[Pluto] Device started successfully" << std::endl;
}

void Pluto::stop() {
    std::cout << "[Pluto] Stopping device" << std::endl;
    
    // Stop the capture thread
    if (running) {
        running = false;
        
        // Properly join the worker thread to ensure clean shutdown
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    std::cout << "[Pluto] Device stopped" << std::endl;
}

void Pluto::process(IqData *buffer1, IqData *buffer2) {
    std::cout << "[Pluto] Starting data processing" << std::endl;
    
    // Start the capture thread - non-blocking, will be joined during stop()
    running = true;
    worker = std::thread(&Pluto::capture_thread, this, buffer1, buffer2);
    
    std::cout << "[Pluto] Data processing thread started" << std::endl;
}

void Pluto::replay(IqData *buffer1, IqData *buffer2, std::string file, bool loop) {
    std::cout << "[Pluto] Starting replay from file: " << file << std::endl;
    
    // Open the file
    std::ifstream replay_file(file, std::ios::binary);
    if (!replay_file.is_open()) {
        std::cerr << "[Pluto] Error: Could not open replay file: " << file << std::endl;
        return;
    }
    
    // Buffer for batch processing
    const size_t BATCH_SIZE = 128;
    std::vector<int16_t> batch_data(BATCH_SIZE * 4); // 4 values (I1,Q1,I2,Q2) per sample
    
    // To track whether we need to exit the loop
    bool should_exit = false;
    running = true; // Use the same flag as regular capture for consistency
    
    while (running && !should_exit) {
        // If we've reached EOF and looping is enabled, start over
        if (replay_file.eof()) {
            if (loop) {
                replay_file.clear();
                replay_file.seekg(0);
                std::cout << "[Pluto] Replay looping back to start of file" << std::endl;
            } else {
                should_exit = true;
                continue;
            }
        }
        
        // Read a batch of samples from file
        replay_file.read(reinterpret_cast<char*>(batch_data.data()), 
                        batch_data.size() * sizeof(int16_t));
        
        // Calculate how many complete samples we actually read
        std::streamsize bytes_read = replay_file.gcount();
        size_t samples_read = bytes_read / (4 * sizeof(int16_t)); // 4 int16_t values per sample
        
        // If we read partial samples, adjust to process only complete ones
        if (bytes_read % (4 * sizeof(int16_t)) != 0) {
            std::cerr << "[Pluto] Warning: Partial sample read from file" << std::endl;
        }
        
        // Process the samples we read
        if (samples_read > 0) {
            // Lock buffers for batch processing
            buffer1->lock();
            buffer2->lock();
            
            for (size_t i = 0; i < samples_read; i++) {
                int16_t i1 = batch_data[i*4 + 0];
                int16_t q1 = batch_data[i*4 + 1];
                int16_t i2 = batch_data[i*4 + 2];
                int16_t q2 = batch_data[i*4 + 3];
                
                // Push to buffers if there's room
                if (buffer1->get_length() < buffer1->get_n() && 
                    buffer2->get_length() < buffer2->get_n()) {
                    buffer1->push_back({static_cast<double>(i1), static_cast<double>(q1)});
                    buffer2->push_back({static_cast<double>(i2), static_cast<double>(q2)});
                } else {
                    // If buffers are full, wait a bit before continuing
                    buffer1->unlock();
                    buffer2->unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    buffer1->lock();
                    buffer2->lock();
                }
            }
            
            // Unlock buffers
            buffer1->unlock();
            buffer2->unlock();
            
        } else if (!replay_file.eof()) {
            // If we didn't read any samples but aren't at EOF, something went wrong
            std::cerr << "[Pluto] Error reading from file" << std::endl;
            should_exit = true;
        }
        
        // Add a small delay to control replay speed and CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    replay_file.close();
    std::cout << "[Pluto] Replay complete" << std::endl;
}
