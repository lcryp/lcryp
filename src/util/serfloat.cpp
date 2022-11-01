#include <util/serfloat.h>
#include <cmath>
#include <limits>
double DecodeDouble(uint64_t v) noexcept {
    static constexpr double NANVAL = std::numeric_limits<double>::quiet_NaN();
    static constexpr double INFVAL = std::numeric_limits<double>::infinity();
    double sign = 1.0;
    if (v & 0x8000000000000000) {
        sign = -1.0;
        v ^= 0x8000000000000000;
    }
    if (v == 0) return copysign(0.0, sign);
    if (v == 0x7ff0000000000000) return copysign(INFVAL, sign);
    int exp = (v & 0x7FF0000000000000) >> 52;
    uint64_t man = v & 0xFFFFFFFFFFFFF;
    if (exp == 2047) {
        return NANVAL;
    } else if (exp == 0) {
        return copysign(ldexp((double)man, -1074), sign);
    } else {
        return copysign(ldexp((double)(man + 0x10000000000000), -1075 + exp), sign);
    }
}
uint64_t EncodeDouble(double f) noexcept {
    int cls = std::fpclassify(f);
    uint64_t sign = 0;
    if (copysign(1.0, f) == -1.0) {
        f = -f;
        sign = 0x8000000000000000;
    }
    if (cls == FP_ZERO) return sign;
    if (cls == FP_INFINITE) return sign | 0x7ff0000000000000;
    if (cls == FP_NAN) return 0x7ff8000000000000;
    int exp;
    uint64_t man = std::round(std::frexp(f, &exp) * 9007199254740992.0);
    if (exp < -1021) {
        if (exp < -1084) return sign;
        return sign | (man >> (-1021 - exp));
    } else {
        if (exp > 1024) return sign | 0x7ff0000000000000;
        return sign | (((uint64_t)(1022 + exp)) << 52) | (man & 0xFFFFFFFFFFFFF);
    }
}
