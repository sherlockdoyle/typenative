#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <unistd.h>

class AdaptiveEstimator {
  static constexpr double EXP_LIMIT = std::numbers::ln2;

  const ssize_t minValue;
  ssize_t curValue;
  double gain = EXP_LIMIT; // the limit is the initial gain

public:
  AdaptiveEstimator(ssize_t minValue) : minValue(minValue), curValue(minValue) {}

  void update(ssize_t newValue) {
    ssize_t delta = newValue - curValue;
    double exp = std::log1p(gain / minValue) * delta;

    bool shouldShrink = exp < -EXP_LIMIT || exp > EXP_LIMIT;
    gain = std::clamp(gain * (shouldShrink ? .9 : 1.1), 1e-15, 1.); // modify gain first

    exp = std::log1p(gain / minValue) * delta; // recompute with new gain
    exp = std::clamp(exp, -EXP_LIMIT, EXP_LIMIT);
    curValue = std::max(minValue, static_cast<ssize_t>(curValue * std::exp(exp)));
  }

  inline ssize_t get() const { return curValue; }
};

class GCStat {
  AdaptiveEstimator youngThreshold{1024}, oldThreshold{65536};

public:
  bool shouldDoYoungGC(size_t youngSize) const { return youngSize > youngThreshold.get(); }
  bool shouldDoOldGC(size_t oldSize) const { return oldSize > oldThreshold.get(); }

  void updateYoung(size_t youngSize) { youngThreshold.update(youngSize); }
  void updateOld(size_t oldSize) { oldThreshold.update(oldSize); }
};