#include "process/conditioning/ArrayReferenceSynthesizer.h"

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <deque>

TEST_CASE("array reference coherently combines two to eight channels")
{
  constexpr std::size_t samples = 1024;
  for (const std::size_t channelCount : {2U, 3U, 5U, 8U})
  {
    std::vector<std::unique_ptr<IqData>> storage;
    std::vector<IqData *> channels;
    for (std::size_t channel = 0; channel < channelCount; channel++)
    {
      auto data = std::make_unique<IqData>(samples);
      std::deque<std::complex<double>> iq;
      for (std::size_t sample = 0; sample < samples; sample++)
        iq.push_back(std::polar(1.0, 0.02 * sample + 0.3 * channel));
      data->replace(std::move(iq));
      channels.push_back(data.get());
      storage.push_back(std::move(data));
    }

    ArrayReferenceSynthesizer::Config config;
    config.analysisSamples = samples;
    config.analysisInterval = 1;
    config.covarianceSmoothing = 0;
    ArrayReferenceSynthesizer synthesizer(config);
    const auto result = synthesizer.process(channels);
    const auto& metrics = synthesizer.get_metrics();

    REQUIRE(result->view_data().size() == samples);
    REQUIRE(metrics.weights.size() == channelCount);
    REQUIRE(metrics.coherentFraction > 0.99);
    REQUIRE(metrics.coherentGainDb >
      10.0 * std::log10(static_cast<double>(channelCount)) - 0.15);
  }
}
