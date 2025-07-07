/// @file Pluto.h
/// @class Pluto
/// @brief A class to capture data on the ADALM-Pluto SDR.
/// @details Uses libiio to communicate with the PlutoSDR device.
/// This implementation supports dual-channel reception for use with
/// passive radar.
/// @author jehan

#ifndef PLUTO_H
#define PLUTO_H

#include "capture/Source.h"
#include "data/IqData.h"

#include <iio.h>
#include <stdint.h>
#include <string>
#include <atomic>
#include <thread>

class Pluto : public Source
{
private:
    /// @brief Device address/URI (e.g. "ip:pluto.local")
    std::string uri;
    
    /// @brief IIO context
    struct iio_context *ctx;
    
    /// @brief AD9361 PHY device
    struct iio_device *phy;
    
    /// @brief RX device
    struct iio_device *rx;
    
    /// @brief RX channels
    struct iio_channel *rx0_i, *rx0_q, *rx1_i, *rx1_q;
    
    /// @brief RX buffer
    struct iio_buffer *rxbuf;
    
    /// @brief Gain mode (manual, slow_attack, fast_attack, hybrid)
    std::string gain_mode;
    
    /// @brief Manual RX gain in dB (0-71dB range)
    int gain_rx;
    
    /// @brief RF port selection ("A_BALANCED", "B_BALANCED", etc.)
    std::string rf_port;
    
    /// @brief RF Bandwidth in Hz
    uint32_t bandwidth;
    
    /// @brief Buffer size in samples
    uint32_t buffer_size;
    
    /// @brief Flag to control the running state
    std::atomic<bool> running;
    
    /// @brief Worker thread for continuous capture
    std::thread worker;
    
    /// @brief Initialize the IIO context and find devices
    /// @return true if successful
    bool init_context();
    
    /// @brief Configure the AD9361 device parameters
    /// @return true if successful
    bool configure_device();
    
    /// @brief Setup the RX channels
    /// @return true if successful
    bool setup_channels();
    
    /// @brief Create the RX buffer
    /// @return true if successful
    bool create_buffer();
    
    /// @brief Worker function for continuous capture
    void capture_thread(IqData *buffer1, IqData *buffer2);
    
    /// @brief Validate parameters for the device
    void validate();
    
    /// @brief Clean up IIO resources
    void cleanup();

public:
    /// @brief Constructor
    /// @param type Device type string
    /// @param fc Center frequency (Hz)
    /// @param fs Sample rate (Hz)
    /// @param path Path to save IQ data
    /// @param saveIq Pointer to save IQ flag
    /// @param uri Device URI (default: "ip:pluto.local")
    /// @param gain_mode Gain mode (default: "manual")
    /// @param gain_rx Manual RX gain (default: 50)
    /// @param rf_port RF port selection (default: "A_BALANCED")
    /// @param bandwidth RF bandwidth (default: 5000000)
    Pluto(std::string type, uint32_t fc, uint32_t fs, 
          std::string path, bool *saveIq,
          std::string uri = "ip:pluto.local",
          std::string gain_mode = "manual",
          int gain_rx = 50,
          std::string rf_port = "A_BALANCED",
          uint32_t bandwidth = 5000000);
    
    /// @brief Destructor - cleanup IIO resources
    ~Pluto();
    
    /// @brief Implement capture function on PlutoSDR
    /// @param buffer1 Pointer to reference buffer
    /// @param buffer2 Pointer to surveillance buffer
    /// @return Void
    void process(IqData *buffer1, IqData *buffer2) override;
    
    /// @brief Call methods to start capture
    /// @return Void
    void start() override;
    
    /// @brief Call methods to gracefully stop capture
    /// @return Void
    void stop() override;
    
    /// @brief Implement replay function
    /// @param buffer1 Pointer to reference buffer
    /// @param buffer2 Pointer to surveillance buffer
    /// @param file Path to file to replay data from
    /// @param loop True if samples should loop at EOF
    /// @return Void
    void replay(IqData *buffer1, IqData *buffer2, std::string file, bool loop) override;

    void configure_rx_channel(struct iio_channel *channel);
};

#endif // PLUTO_H
