#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"

#include "Ambiguity.h"
#include <random>

std::random_device g_rd;

using namespace Catch::literals;

// Have to use out ref parameter because there's no copy/move ctors
void random_iq(IqData& iq_data) {
    std::mt19937 gen(g_rd());
    std::uniform_real_distribution<> dist(-100.0, 100.0);

    for (uint32_t i = 0; i < iq_data.get_n(); ++i) {
        iq_data.push_back({dist(gen), dist(gen)});
    }
}

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

    // Only read for the buffer length - this class is very poorly designed.
    if (buffer1.get_length() == buffer1.get_n()) {
        break;
    }
  }

  fclose(file_replay);
}

// Make sure the constructor is calculating the parameters correctly.
TEST_CASE("Constructor", "[constructor]")
{
    int32_t delay_min{-10};
    int32_t delay_max{300};
    int32_t doppler_min{-300};
    int32_t doppler_max{300};

    uint32_t fs{2'000'000};
    float cpi_s{0.5};
    uint32_t n_samples = cpi_s * fs;    // narrow on purpose

    Ambiguity ambiguity(delay_min,delay_max,doppler_min,doppler_max,fs,n_samples);

    CHECK_THAT(ambiguity.cpi_length_seconds(), Catch::Matchers::WithinAbs(cpi_s, 0.02));
    CHECK(ambiguity.doppler_middle() == 0);
    CHECK(ambiguity.corr_samples_per_pulse() == 3322);
    CHECK(ambiguity.delay_bin_count() == delay_max + std::abs(delay_min) + 1);
    CHECK(ambiguity.doppler_bin_count() == 301);
    CHECK(ambiguity.fft_bin_count() == 6643);
}

// Make sure process produces an output
TEST_CASE("Process_Simple", "[process]")
{
    int32_t delay_min{-10};
    int32_t delay_max{300};
    int32_t doppler_min{-300};
    int32_t doppler_max{300};

    uint32_t fs{2'000'000};
    float cpi_s{0.5};
    uint32_t n_samples = cpi_s * fs;    // narrow on purpose

    Ambiguity ambiguity(delay_min,delay_max,doppler_min,doppler_max,fs,n_samples);

    IqData x{n_samples};
    IqData y{n_samples};

    random_iq(x);
    random_iq(y);
    auto map{ambiguity.process(&x, &y)};
    map->set_metrics();
    CHECK(map->maxPower > 0.0);
    CHECK(map->noisePower > 0.0);
}

// Sanity check that we're getting numbers close to the baseline ambiguity processing function.
TEST_CASE("Process_File", "[process]")
{
    int32_t delay_min{-10};
    int32_t delay_max{300};
    int32_t doppler_min{-300};
    int32_t doppler_max{300};

    uint32_t fs{2'000'000};
    float cpi_s{0.5};
    uint32_t n_samples = cpi_s * fs;    // narrow on purpose

    Ambiguity ambiguity(delay_min,delay_max,doppler_min,doppler_max,fs,n_samples);
    IqData x{n_samples};
    IqData y{n_samples};

    read_file(x, y, "20231214-230611.rspduo");
    REQUIRE(x.get_length() == x.get_n());

    auto map{ambiguity.process(&x ,&y)};
    map->set_metrics();
    CHECK_THAT(map->maxPower, Catch::Matchers::WithinAbs(30.2816, 0.001));
    CHECK_THAT(map->noisePower, Catch::Matchers::WithinAbs(76.918, 0.001));
}