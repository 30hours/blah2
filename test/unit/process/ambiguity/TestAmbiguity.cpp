/// @file TestAmbiguity.cpp
/// @brief Unit test for Ambiguity.cpp
/// @author 30hours
/// @author Dan G
/// @todo Add golden data IqData file for testing.
/// @todo Declaration match to coding style?

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "process/ambiguity/Ambiguity.h"

#include <random>
#include <iostream>
#include <filesystem>

/// @brief Use random_device as RNG.
std::random_device g_rd;

/// @brief Generate random IQ data.
/// @param iqData Address of IqData object.
/// @details Have to use out ref parameter because there's no copy/move ctors.
/// @return Void.
void random_iq(IqData& iq_data) {
    std::mt19937 gen(g_rd());
    std::uniform_real_distribution<> dist(-100.0, 100.0);

    for (uint32_t i = 0; i < iq_data.get_n(); ++i) {
        iq_data.push_back({dist(gen), dist(gen)});
    }
}

/// @brief Read file to IqData buffer.
/// @param buffer1 IqData buffer reference.
/// @param buffer2 IqData buffer surveillance.
/// @param file String of file name.
/// @return Void.
void read_file(IqData& buffer1, IqData& buffer2, const std::string& file)
{
  short i1, q1, i2, q2;
  auto file_replay = fopen(file.c_str(), "rb");
  if (!file_replay) {
    return;
  }

  auto read_short = [](short& v, FILE* fid) {
    auto rv{fread(&v, 1, sizeof(short), fid)};
    return rv == sizeof(short);
  };

  while (!feof(file_replay))
  {
    if (!read_short(i1, file_replay)) break;
    if (!read_short(q1, file_replay)) break;
    if (!read_short(i2, file_replay)) break;
    if (!read_short(q2, file_replay)) break;

    buffer1.push_back({(double)i1, (double)q1});
    buffer2.push_back({(double)i2, (double)q2});

    // only read for the buffer length - this class is very poorly designed
    if (buffer1.get_length() == buffer1.get_n()) {
        break;
    }
  }

  fclose(file_replay);
}

/// @brief Test constructor.
/// @details Check constructor parameters created correctly.
TEST_CASE("Constructor", "[constructor]")
{
    int32_t delayMin{-10};
    int32_t delayMax{300};
    int32_t dopplerMin{-300};
    int32_t dopplerMax{300};

    uint32_t fs{2'000'000};
    float tCpi{0.5};
    uint32_t nSamples = tCpi * fs;    // narrow on purpose

    Ambiguity ambiguity(delayMin, delayMax, dopplerMin, 
      dopplerMax, fs, nSamples);

    CHECK_THAT(ambiguity.cpi_length_seconds(), Catch::Matchers::WithinAbs(tCpi, 0.02));
    CHECK(ambiguity.doppler_middle() == 0);
    CHECK(ambiguity.corr_samples_per_pulse() == 3322);
    CHECK(ambiguity.delay_bin_count() == delayMax + std::abs(delayMin) + 1);
    CHECK(ambiguity.doppler_bin_count() == 301);
    CHECK(ambiguity.fft_bin_count() == 6643);
}

/// @brief Test constructor with rounded Hamming number FFT length.
TEST_CASE("Constructor_Round", "[constructor]")
{
    int32_t delayMin{-10};
    int32_t delayMax{300};
    int32_t dopplerMin{-300};
    int32_t dopplerMax{300};

    uint32_t fs{2'000'000};
    float tCpi{0.5};
    uint32_t nSamples = tCpi * fs;    // narrow on purpose

    Ambiguity ambiguity(delayMin, delayMax, dopplerMin, 
      dopplerMax, fs, nSamples, true);

    CHECK_THAT(ambiguity.cpi_length_seconds(), Catch::Matchers::WithinAbs(tCpi, 0.02));
    CHECK(ambiguity.doppler_middle() == 0);
    CHECK(ambiguity.corr_samples_per_pulse() == 3322);
    CHECK(ambiguity.delay_bin_count() == delayMax + std::abs(delayMin) + 1);
    CHECK(ambiguity.doppler_bin_count() == 301);
    CHECK(ambiguity.fft_bin_count() == 6750);
}

/// @brief Test simple ambiguity processing.
TEST_CASE("Process_Simple", "[process]")
{
    auto round_hamming = GENERATE(true, false);

    int32_t delayMin{-10};
    int32_t delayMax{300};
    int32_t dopplerMin{-300};
    int32_t dopplerMax{300};

    uint32_t fs{2'000'000};
    float tCpi{0.5};
    uint32_t nSamples = tCpi * fs;    // narrow on purpose

    Ambiguity ambiguity(delayMin, delayMax, dopplerMin, 
      dopplerMax, fs, nSamples, round_hamming);

    IqData x{nSamples};
    IqData y{nSamples};

    random_iq(x);
    random_iq(y);
    auto map{ambiguity.process(&x, &y)};
    map->set_metrics();
    CHECK(map->maxPower > 0.0);
    CHECK(map->noisePower > 0.0);

    std::cout << "Process_Simple with" << (round_hamming ? " hamming\n" : "out hamming\n")
              << ambiguity.get_latest_performance() << "\n-----------" << std::endl;
}

/// @brief Test processing from a file.
TEST_CASE("Process_File", "[process]")
{
    std::filesystem::path test_input_file("20231214-230611.rspduo");
    // Bail if the test file doesn't exist
    if (!std::filesystem::exists(test_input_file)) {
      SKIP("Input test file does not exist.");
    }
    
    auto round_hamming = GENERATE(true, false);

    int32_t delayMin{-10};
    int32_t delayMax{300};
    int32_t dopplerMin{-300};
    int32_t dopplerMax{300};

    uint32_t fs{2'000'000};
    float tCpi{0.5};
    uint32_t nSamples = tCpi * fs;    // narrow on purpose

    Ambiguity ambiguity(delayMin, delayMax, dopplerMin, 
      dopplerMax, fs, nSamples, round_hamming);
    IqData x{nSamples};
    IqData y{nSamples};

    read_file(x, y, "20231214-230611.rspduo");
    REQUIRE(x.get_length() == x.get_n());

    auto map{ambiguity.process(&x ,&y)};
    map->set_metrics();
    CHECK_THAT(map->maxPower, Catch::Matchers::WithinAbs(30.2816, 0.001));
    CHECK_THAT(map->noisePower, Catch::Matchers::WithinAbs(76.918, 0.001));

    std::cout << "Process_File with" << (round_hamming ? " hamming\n" : "out hamming\n")
              << ambiguity.get_latest_performance() << "\n-----------" << std::endl;
}

/// @brief Test Hamming number calculation.
TEST_CASE("Next_Hamming", "[hamming]")
{
    CHECK(next_hamming(104) == 108);
    CHECK(next_hamming(3322) == 3375);
    CHECK(next_hamming(19043) == 19200);
}