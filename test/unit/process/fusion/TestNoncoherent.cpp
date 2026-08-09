#include "process/fusion/Noncoherent.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

TEST_CASE("noncoherent fusion preserves RMS magnitude")
{
  Map<std::complex<double>> first(1, 1);
  Map<std::complex<double>> second(1, 1);
  first.data[0][0] = {3, 0};
  second.data[0][0] = {0, 4};
  first.delay.push_back(2);
  first.doppler.push_back(5);

  Noncoherent fusion;
  const auto result = fusion.process({&first, &second});
  REQUIRE(result->data[0][0].real() == Catch::Approx(std::sqrt(12.5)));
  REQUIRE(result->data[0][0].imag() == 0);
  REQUIRE(result->delay.front() == 2);
  REQUIRE(result->doppler.front() == 5);
}
