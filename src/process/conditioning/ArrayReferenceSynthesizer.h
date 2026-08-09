#ifndef ARRAY_REFERENCE_SYNTHESIZER_H
#define ARRAY_REFERENCE_SYNTHESIZER_H

#include "data/IqData.h"

#include <complex>
#include <cstdint>
#include <memory>
#include <vector>

class ArrayReferenceSynthesizer
{
public:
  struct Config
  {
    uint32_t analysisSamples = 32768;
    uint32_t analysisInterval = 10;
    uint32_t powerIterations = 12;
    double covarianceSmoothing = 0.8;
    double diagonalLoading = 0.001;
  };

  struct Metrics
  {
    double coherentFraction = 0;
    double coherentGainDb = 0;
    uint64_t updates = 0;
    std::vector<std::complex<double>> weights;
  };

  ArrayReferenceSynthesizer();
  explicit ArrayReferenceSynthesizer(Config config);
  std::unique_ptr<IqData> process(const std::vector<IqData *>& channels);
  const Metrics& get_metrics() const { return metrics; }

private:
  Config config;
  Metrics metrics;
  uint64_t frames = 0;
};

#endif
