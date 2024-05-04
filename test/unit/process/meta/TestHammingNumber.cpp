/// @file TestHammingNumber.cpp
/// @brief Unit test for HammingNumber.cpp
/// @author 30hours
/// @author Dan G

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "process/meta/HammingNumber.h"

/// @brief Test Hamming number calculation.
TEST_CASE("Next_Hamming", "[hamming]")
{
    CHECK(next_hamming(104) == 108);
    CHECK(next_hamming(3322) == 3375);
    CHECK(next_hamming(19043) == 19200);
}