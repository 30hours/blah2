#ifndef NONCOHERENT_H
#define NONCOHERENT_H

#include "data/Map.h"

#include <complex>
#include <memory>
#include <vector>

class Noncoherent
{
public:
  std::unique_ptr<Map<std::complex<double>>> process(
    const std::vector<Map<std::complex<double>> *>& maps) const;
};

#endif
