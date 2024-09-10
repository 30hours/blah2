#include "HammingNumber.h"

bool HammingNumber::operator!=(const HammingNumber &other) const
{
  return true;
}

HammingNumber HammingNumber::begin() const
{
  return *this;
}

HammingNumber HammingNumber::end() const
{
  return *this;
}

unsigned int HammingNumber::operator*() const
{
  return x.back();
}

HammingNumber::HammingNumber(const std::vector<unsigned int> &pfs)
    : H(pfs), hp(pfs.size(), 0), hv({pfs}), x({1}) {}

const HammingNumber &HammingNumber::operator++()
{
  for (std::vector<unsigned int>::size_type i = 0; i < H.size(); i++)
    for (; hv[i] <= x.back(); hv[i] = x[++hp[i]] * H[i])
      ;
  x.push_back(hv[0]);
  for (std::vector<unsigned int>::size_type i = 1; i < H.size(); i++)
    if (hv[i] < x.back())
      x.back() = hv[i];
  return *this;
}

uint32_t next_hamming(uint32_t value)
{
  for (auto i : HammingNumber({2, 3, 5}))
  {
    if (i > value)
    {
      return i;
    }
  }
  return 0;
}
