/// @file Conversions.h
/// @class Conversions
/// @brief A class to convert between different units.

/// @author bennysomers
/// @todo Add more conversions

#ifndef CONVERSIONS_H
#define CONVERSIONS_H

#include <math.h>

class Conversions
{
public:
    /// @brief Convert from dB to linear.
    /// @param db Value in dB.
    /// @return Value in linear.
    static double db2lin(double db);

    /// @brief Convert from linear to dB.
    /// @param lin Value in linear.
    /// @return Value in dB.
    static double lin2db(double lin);
};

#endif