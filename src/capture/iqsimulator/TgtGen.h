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

#include <ryml/ryml.hpp>
#include <ryml/ryml_std.hpp>
#include <c4/format.hpp>

#include <stdint.h>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <complex>

class FalseTarget
{
private:
    /// @brief fs
    uint32_t fs;

    /// @brief Target type.
    std::string type;

    /// @brief Target delay
    double delay;

    /// @brief Target delay in samples
    uint32_t delay_samples;

    /// @brief Target range
    double range;

    /// @brief Target Doppler
    double doppler;

    /// @brief Target RCS
    double rcs;

    /// @brief Target ID
    u_int32_t id;

public:
    /// @brief Constructor for targets.
    /// @return The object.
    FalseTarget(c4::yml::NodeRef target_node, uint32_t _fs);

    /// @brief Generate the signal from a false target.
    /// @param buffer Pointer to reference buffer.
    /// @return Target reflection signal.
    std::complex<double> process(IqData *buffer);

    /// @brief Getter for target type.
    /// @return Target type.
    std::string get_type();

    /// @brief Getter for target range.type
    /// @return Target range.
    double get_range();

    /// @brief Getter for target Doppler.
    /// @return Target Doppler.
    double get_doppler();

    /// @brief Getter for target RCS.
    /// @return Target RCS.
    double get_rcs();

    /// @brief Getter for target delay.
    /// @return Target delay.
    double get_delay();

    /// @brief Getter for target id.
    /// @return Target id.
    u_int32_t get_id();
};

class TgtGen
{
private:
    /// @brief Vector of false targets.
    std::vector<FalseTarget> targets;

public:
    /// @brief The valid false target types.
    static const std::string VALID_TYPE[1];

    /// @brief The valid false target states.
    static const std::string VALID_STATE[1];

    /// @brief Constructor.
    /// @return The object.
    TgtGen(std::string configPath, uint32_t fs);

    /// @brief Generate the signal from all false targets.
    /// @param ref_buffer Pointer to reference buffer.
    /// @return Targets reflection signal.
    std::complex<double> process(IqData *ref_buffer);
};

#endif
std::string ryml_get_file(const char *filename);