/// @file HammingNumber.h
/// @class HammingNumber
/// @brief Hamming number generator
/// @author Nigel Galloway
/// @cite https://rosettacode.org/wiki/Hamming_numbers

#ifndef HAMMING_GENERATOR_H
#define HAMMING_GENERATOR_H

#include <vector>
#include <stdint.h>

class HammingNumber 
{

  private:
      std::vector<unsigned int> H, hp, hv, x;

  public:
      bool operator!=(const HammingNumber &other) const;
      HammingNumber begin() const;
      HammingNumber end() const;
      unsigned int operator*() const;
      HammingNumber(const std::vector<unsigned int> &pfs);
      const HammingNumber &operator++();

};

/// @brief  Calculate the next 5-smooth Hamming Number larger than value
/// @param value Value to round
/// @return value rounded to Hamming number
uint32_t next_hamming(uint32_t value);

#endif