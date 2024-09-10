#include "Kraken.h"

#include <iostream>
#include <complex>
#include <thread>
#include <algorithm>

// constructor
Kraken::Kraken(std::string _type, uint32_t _fc, uint32_t _fs, 
  std::string _path, bool *_saveIq, std::vector<double> _gain)
    : Source(_type, _fc, _fs, _path, _saveIq)
{
    // convert gain to tenths of dB
    for (size_t i = 0; i < _gain.size(); i++)
    {
        gain.push_back(static_cast<int>(_gain[i]*10));
        channelIndex.push_back(i);
    }
    std::vector<rtlsdr_dev_t*> devs(channelIndex.size());

    // store all valid gains
    std::vector<int> validGains;
    int nGains, status;
    status = rtlsdr_open(&devs[0], 0);
    check_status(status, "Failed to open device for available gains.");
    nGains = rtlsdr_get_tuner_gains(devs[0], nullptr);
    check_status(nGains, "Failed to get number of gains.");
    std::unique_ptr<int[]> _validGains(new int[nGains]);
    status = rtlsdr_get_tuner_gains(devs[0], _validGains.get());
    check_status(status, "Failed to get number of gains.");
    validGains.assign(_validGains.get(), _validGains.get() + nGains);
    status = rtlsdr_close(devs[0]);
    check_status(status, "Failed to close device for available gains.");

    // update gains to next value if invalid
    for (size_t i = 0; i < _gain.size(); i++)
    {
        int adjustedGain = static_cast<int>(_gain[i] * 10);
        auto it = std::lower_bound(validGains.begin(), 
            validGains.end(), adjustedGain);
        if (it != validGains.end()) {
            gain.push_back(*it);
        } else {
            gain.push_back(validGains.back());
        }
        std::cout << "[Kraken] Gain update on channel " << i << " from " << 
            adjustedGain << " to " << gain[i] << "." << std::endl;
    }
}

void Kraken::start()
{
    int status;
    for (size_t i = 0; i < channelIndex.size(); i++) 
    {
        std::cout << "[Kraken] Setting up channel " << i << "." << std::endl;

        status = rtlsdr_open(&devs[i], i);
        check_status(status, "Failed to open device.");

        status = rtlsdr_set_center_freq(devs[i], fc);
        check_status(status, "Failed to set center frequency.");
        status = rtlsdr_set_sample_rate(devs[i], fs);
        check_status(status, "Failed to set sample rate.");
        status = rtlsdr_set_dithering(devs[i], 0); // disable dither
        check_status(status, "Failed to disable dithering.");
        status = rtlsdr_set_tuner_gain_mode(devs[i], 1); // disable AGC
        check_status(status, "Failed to disable AGC.");
        status = rtlsdr_set_tuner_gain(devs[i], gain[i]);
        check_status(status, "Failed to set gain.");
        status = rtlsdr_reset_buffer(devs[i]);
        check_status(status, "Failed to reset buffer.");
    }
}

void Kraken::stop()
{
    int status;
    for (size_t i = 0; i < channelIndex.size(); i++) 
    {
        status = rtlsdr_cancel_async(devs[i]);
        check_status(status, "Failed to stop async read.");
    }
}

void Kraken::process(IqData *buffer1, IqData *buffer2)
{
    std::vector<std::thread> threads;
    threads.emplace_back(rtlsdr_read_async, devs[0], callback, buffer1, 0, 16 * 16384);
    threads.emplace_back(rtlsdr_read_async, devs[1], callback, buffer2, 0, 16 * 16384);
    // join threads
    for (auto& thread : threads) {
        thread.join();
    }
}

void Kraken::callback(unsigned char *buf, uint32_t len, void *ctx) 
{
    IqData* buffer_blah2 = (IqData*)ctx;
    int8_t* buffer_kraken = (int8_t*)buf;

    buffer_blah2->lock();

    for (size_t i = 0; i < len; i += 2) {
        double iqi = static_cast<double>(buffer_kraken[i]);
        double iqq = static_cast<double>(buffer_kraken[i + 1]);

        buffer_blah2->push_back({iqi, iqq});
    }

    buffer_blah2->unlock();
}

void Kraken::replay(IqData *buffer1, IqData *buffer2, std::string _file, bool _loop)
{
    // todo
}

void Kraken::check_status(int status, std::string message)
{
  if (status < 0)
  {
    throw std::runtime_error("[Kraken] " + message);
  }
}
