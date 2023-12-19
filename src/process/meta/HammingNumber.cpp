#include "HammingNumber.h"

bool HammingNumber::operator!=(const HammingNumber &other) const {
    return true;
}

HammingNumber HammingNumber::begin() const {
    return *this;
}

HammingNumber HammingNumber::end() const {
    return *this;
}

unsigned int HammingNumber::operator*() const {
    return _x.back();
}

HammingNumber::HammingNumber(const std::vector<unsigned int> &pfs)
    : _H(pfs), _hp(pfs.size(), 0), _hv({pfs}), _x({1}) {}

const HammingNumber &HammingNumber::operator++() {
    for (int i = 0; i < _H.size(); i++)
        for (; _hv[i] <= _x.back(); _hv[i] = _x[++_hp[i]] * _H[i])
            ;
    _x.push_back(_hv[0]);
    for (int i = 1; i < _H.size(); i++)
        if (_hv[i] < _x.back())
            _x.back() = _hv[i];
    return *this;
}

uint32_t next_hamming(uint32_t value) {
    for (auto i : HammingNumber({2, 3, 5})) {
        if (i > value) {
            return i;
        }
    }
    return 0;
}
