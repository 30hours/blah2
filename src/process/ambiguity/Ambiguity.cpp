#include "Ambiguity.h"
#include <complex>
#include <iostream>
#include <deque>
#include <vector>
#include <numeric>
#include <math.h>
#include <chrono>

// constructor
Ambiguity::Ambiguity(int32_t delayMin, int32_t delayMax, int32_t dopplerMin, int32_t dopplerMax, uint32_t fs, uint32_t n, bool roundHamming)
  : delayMin_{delayMin}
  , delayMax_{delayMax}
  , dopplerMin_{dopplerMin}
  , dopplerMax_{dopplerMax}
  , fs_{fs}
  , nSamples_{n}
  , nDelayBins_{static_cast<uint16_t>(delayMax - delayMin + 1)} // If delayMin > delayMax = trouble, what's the exception policy?
  , dopplerMiddle_{(dopplerMin_ + dopplerMax_) / 2.0}
{
  // doppler calculations
  std::deque<double> doppler;
  double resolutionDoppler = 1.0 / (static_cast<double>(n) / static_cast<double>(fs));
  doppler.push_back(dopplerMiddle_);
  int i = 1;
  while (dopplerMiddle_ + (i * resolutionDoppler) <= dopplerMax)
  {
    doppler.push_back(dopplerMiddle_ + (i * resolutionDoppler));
    doppler.push_front(dopplerMiddle_ - (i * resolutionDoppler));
    i++;
  }
  nDopplerBins_ = doppler.size();

  // batches constants
  nCorr_ = n / nDopplerBins_;
  cpi_ = (static_cast<double>(nCorr_) * nDopplerBins_) / fs;

  // update doppler bins to true cpi time
  resolutionDoppler = 1.0 / cpi_;

  // create ambiguity map
  map_ = std::make_unique<Map<Complex>>(nDopplerBins_, nDelayBins_);

  // delay calculations
  map_->delay.resize(nDelayBins_);
  std::iota(map_->delay.begin(), map_->delay.end(), delayMin_);

  map_->doppler.push_front(dopplerMiddle_);
  i = 1;
  while (map_->doppler.size() < nDopplerBins_)
  {
    map_->doppler.push_back(dopplerMiddle_ + (i * resolutionDoppler));
    map_->doppler.push_front(dopplerMiddle_ - (i * resolutionDoppler));
    i++;
  }

  // other setup
  nfft_ = 2 * nCorr_ - 1;
  if (roundHamming) {
    nfft_ = next_hamming(nfft_);
  }
  dataCorr_.resize(2 * nDelayBins_ + 1);

  // compute FFTW plans in constructor
  dataXi_.resize(nfft_);
  dataYi_.resize(nfft_);
  dataZi_.resize(nfft_);
  dataDoppler_.resize(nfft_);
  fftXi_ = fftw_plan_dft_1d(nfft_, reinterpret_cast<fftw_complex *>(dataXi_.data()),
                           reinterpret_cast<fftw_complex *>(dataXi_.data()), FFTW_FORWARD, FFTW_ESTIMATE);
  fftYi_ = fftw_plan_dft_1d(nfft_, reinterpret_cast<fftw_complex *>(dataYi_.data()),
                           reinterpret_cast<fftw_complex *>(dataYi_.data()), FFTW_FORWARD, FFTW_ESTIMATE);
  fftZi_ = fftw_plan_dft_1d(nfft_, reinterpret_cast<fftw_complex *>(dataZi_.data()),
                           reinterpret_cast<fftw_complex *>(dataZi_.data()), FFTW_BACKWARD, FFTW_ESTIMATE);
  fftDoppler_ = fftw_plan_dft_1d(nDopplerBins_, reinterpret_cast<fftw_complex *>(dataDoppler_.data()),
                                reinterpret_cast<fftw_complex *>(dataDoppler_.data()), FFTW_FORWARD, FFTW_ESTIMATE);

}

Ambiguity::~Ambiguity()
{
  fftw_destroy_plan(fftXi_);
  fftw_destroy_plan(fftYi_);
  fftw_destroy_plan(fftZi_);
  fftw_destroy_plan(fftDoppler_);
}

Map<std::complex<double>> *Ambiguity::process(IqData *x, IqData *y)
{
  using Timer = std::chrono::steady_clock;
  auto t0{Timer::now()};
  Timer::duration range_fft_dur{};

  // shift reference if not 0 centered
  if (dopplerMiddle_ != 0)
  {
    std::complex<double> j = {0, 1};
    for (int i = 0; i < x->get_length(); i++)
    {
      x->push_back(x->pop_front() * std::exp(1.0 * j * 2.0 * M_PI * dopplerMiddle_ * ((double)i / fs_)));
    }
  }

  // range processing
  for (int i = 0; i < nDopplerBins_; i++)
  {
    for (int j = 0; j < nCorr_; j++)
    {
      dataXi_[j] = x->pop_front();
      dataYi_[j] = y->pop_front();
    }

    for (int j = nCorr_; j < nfft_; j++)
    {
      dataXi_[j] = {0, 0};
      dataYi_[j] = {0, 0};
    }

    auto t1{Timer::now()};
    fftw_execute(fftXi_);
    fftw_execute(fftYi_);
    range_fft_dur += Timer::now() - t1;

    // compute correlation
    for (int j = 0; j < nfft_; j++)
    {
      dataZi_[j] = (dataYi_[j] * std::conj(dataXi_[j])) / (double)nfft_;
    }

    t1 = Timer::now();
    fftw_execute(fftZi_);
    range_fft_dur += Timer::now() - t1;

    // extract center of corr
    for (int j = 0; j < nDelayBins_; j++)
    {
      dataCorr_[j] = dataZi_[nfft_ - nDelayBins_ + j];
    }
    for (int j = 0; j < nDelayBins_ + 1; j++)
    {
      dataCorr_[j + nDelayBins_] = dataZi_[j];
    }

    // cast from std::complex to std::vector
    corr_.clear();
    for (int j = 0; j < nDelayBins_; j++)
    {
      corr_.push_back(dataCorr_[nDelayBins_ + delayMin_ + j - 1 + 1]);
    }

    map_->set_row(i, corr_);
  }

  // doppler processing
  auto t1{Timer::now()};
  for (int i = 0; i < nDelayBins_; i++)
  {
    delayProfile_ = map_->get_col(i);
    for (int j = 0; j < nDopplerBins_; j++)
    {
      dataDoppler_[j] = {delayProfile_[j].real(), delayProfile_[j].imag()};
    }

    fftw_execute(fftDoppler_);

    corr_.clear();
    for (int j = 0; j < nDopplerBins_; j++)
    {
      corr_.push_back(dataDoppler_[(j + int(nDopplerBins_ / 2) + 1) % nDopplerBins_]);
    }

    map_->set_col(i, corr_);
  }

  auto to_ms = [] (const Timer::duration& dur) {
    return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(dur).count();
  };

  latest_performance_.process_time_ms = to_ms(Timer::now() - t0);
  latest_performance_.doppler_fft_time_ms = to_ms(Timer::now() - t1);
  latest_performance_.range_fft_time_ms = to_ms(range_fft_dur);

  return map_.get();
}

/**
 * @brief Hamming number generator
 *
 * @author Nigel Galloway
 * @cite https://rosettacode.org/wiki/Hamming_numbers
 * @todo Can this be done with constexpr???
 */
class HammingGenerator {
private:
	std::vector<unsigned int> _H, _hp, _hv, _x;
public:
  bool operator!=(const HammingGenerator &other) const { return true; }
  HammingGenerator begin() const { return *this; }
  HammingGenerator end() const { return *this; }
  unsigned int operator*() const { return _x.back(); }
  HammingGenerator(const std::vector<unsigned int> &pfs) : _H(pfs), _hp(pfs.size(), 0), _hv({pfs}), _x({1}) {}
  const HammingGenerator &operator++()
  {
    for (int i = 0; i < _H.size(); i++)
      for (; _hv[i] <= _x.back(); _hv[i] = _x[++_hp[i]] * _H[i])
        ;
    _x.push_back(_hv[0]);
    for (int i = 1; i < _H.size(); i++)
      if (_hv[i] < _x.back())
        _x.back() = _hv[i];
    return *this;
  }
};


uint32_t next_hamming(uint32_t value) {
  for (auto i : HammingGenerator({2,3,5})) {
    if (i > value) {
      return i;
    }
  }
  return 0;
}

std::ostream& operator<<(std::ostream& str, const Ambiguity::PerformanceStats& stats) {
  return str << "Total time: " << stats.process_time_ms << "ms\n" <<
                "Range FFT time: " << stats.range_fft_time_ms << "ms\n" <<
                "Doppler FFT time: " << stats.doppler_fft_time_ms << "ms";
}