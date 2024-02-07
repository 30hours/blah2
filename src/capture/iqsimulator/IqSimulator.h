/// @file IqSimulator.h
/// @class IqSimulator
/// @brief A class to generate simulated IQ data with false targets
/// @details This class generates simulated IQ data with false targets.
/// It generates a random reference and surveillance signal and uses the
/// TgtGen class to add false targets to the surveillance signal.
///
/// @author bennysomers
/// @todo Simulate a single false target
/// @todo Simulate false targets
/// @todo Simulate realistic target motion
/// @todo Simulate N channels, instead of just 2

#ifndef IQSIMULATOR_H
#define IQSIMULATOR_H

#include "capture/Source.h"
#include "TgtGen.h"
#include "data/IqData.h"

#include <stdint.h>
#include <string>
#include <iostream>
#include <vector>
#include <complex>
#include <random>

class IqSimulator : public Source
{
private:
    /// @brief Number of samples to generate each loop.
    /// @details This is the threshold for the minimum number of samples
    /// left in the buffer before new samples will be generated.
    uint32_t n_min;

    /// @brief Total number of samples generated.
    /// @details This is used to keep track of the total number of samples
    /// generated, so that the Doppler shift can be calculated.
    u_int64_t total_samples;

    /// @brief Path to the false targets configuration file.
    std::string false_targets_config_file_path;

    /// @brief Path to the radar configuration file.
    std::string config_file_path;

public:
    /// @brief Constructor.
    /// @param type Type of source. = "IQSimulator"
    /// @param fc Center frequency (Hz).
    /// @param fs Sample rate (Hz).
    /// @param path Path to save IQ data.
    /// @param saveIq Pointer to boolean to save IQ data.
    /// @param n Number of samples.
    /// @return The object.
    IqSimulator(std::string type, uint32_t fc, uint32_t fs, std::string path,
                bool *saveIq, uint32_t n_min,
                std::string false_targets_config_file_path = "config/false_targets.yml",
                std::string config_file_path = "config/config.yml");

    /// @brief Implement capture function on IQSimulator.
    /// @param buffer1 Pointer to reference buffer.
    /// @param buffer2 Pointer to surveillance buffer.
    /// @return Void.
    void process(IqData *buffer1, IqData *buffer2);

    /// @brief Call methods to start capture.
    /// @return Void.
    void start();

    /// @brief Call methods to gracefully stop capture.
    /// @return Void.
    void stop();

    /// @brief Implement replay function on IQSimulator.
    /// @param buffer1 Pointer to reference buffer.
    /// @param buffer2 Pointer to surveillance buffer.
    /// @param file Path to file to replay data from.
    /// @param loop True if samples should loop at EOF.
    /// @return Void.
    void replay(IqData *buffer1, IqData *buffer2, std::string file, bool loop);
};

#endif