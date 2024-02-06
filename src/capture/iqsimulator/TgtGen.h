/// @file TgtGen.h
/// @class TgtGen
/// @brief A class to generate false targets.

/// @details
/// Static Targets: remain at a fixed range/delay and Doppler.

/// @author bennysomers
/// @todo Simulate a false target moving in radar coordinates
/// @todo Simulate a false target moving in spatial coordinates

#ifndef TGTGEN_H
#define TGTGEN_H

#include "data/IqData.h"
#include "utilities/Conversions.h"
#include "data/meta/Constants.h"
#include "process/utility/Socket.h"

#include <ryml/ryml.hpp>
#include <ryml/ryml_std.hpp>
#include <c4/format.hpp>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filewritestream.h"

#include <stdint.h>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <complex>
#include <math.h>

#include <iomanip>

class FalseTarget
{
private:
    /// @brief fs
    uint32_t fs;

    /// @brief fc
    uint32_t fc;

    /// @brief Target delay
    double delay;

    /// @brief Target delay in samples
    uint32_t delay_samples;

    /// @brief Target range
    double range;

    /// @brief Target starting range
    double start_range;

    /// @brief Sample counter
    uint64_t sample_counter;

public:
    /// @brief Target type.
    std::string type;

    /// @brief Target Doppler
    double doppler;

    /// @brief Target Doppler Rate
    double doppler_rate;

    /// @brief Target RCS
    double rcs;

    /// @brief Target ID
    u_int32_t id;

    /// @brief Constructor for targets.
    /// @return The object.
    FalseTarget(c4::yml::NodeRef target_node, uint32_t _fs, uint32_t _fc);

    /// @brief Generate the signal from a false target.
    /// @param ref_buffer Pointer to reference buffer.
    /// @return Target reflection signal.
    std::complex<double> process(IqData *ref_buffer);

    /// @brief Getter for range.
    /// @return Range in meters.
    double get_range();

    /// @brief Setter for range.
    /// @param range Range in meters.
    void set_range(double range);

    /// @brief Getter for delay.
    /// @return Delay in seconds.
    double get_delay();

    /// @brief Setter for delay.
    /// @param delay Delay in seconds.
    void set_delay(double delay);

    /// @brief Outputs false target truth as JSON
    /// @return JSON string.
    rapidjson::Value to_json(rapidjson::Document::AllocatorType &allocator);
};

class TgtGen
{
private:
    /// @brief Vector of false targets.
    std::vector<FalseTarget> targets;

    /// @brief Socket to send false target data.
    Socket *socket;

    /// @brief Sample counter
    uint64_t sample_counter;

public:
    /// @brief The valid false target types.
    static const std::string VALID_TYPE[2];

    /// @brief The valid false target states.
    static const std::string VALID_STATE[1];

    /// @brief Constructor.
    /// @param false_tgt_config_path Path to false targets configuration file.
    /// @param config_path Path to blah2 config file.
    /// @param fs Sample rate (Hz).
    /// @param fc Center frequency (Hz).
    /// @return The object.
    TgtGen(std::string false_tgt_config_path, std::string config_path, uint32_t fs, uint32_t fc);

    /// @brief Generate the signal from all false targets.
    /// @param ref_buffer Pointer to reference buffer.
    /// @return Targets reflection signal.
    std::complex<double> process(IqData *ref_buffer);
};

#endif
std::string ryml_get_file(const char *filename);