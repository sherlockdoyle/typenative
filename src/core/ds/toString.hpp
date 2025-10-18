#pragma once

#include "../rt/AutoRef.hpp"
#include "valTypes.hpp"

template <typename T> inline std::string toString(const T &t) noexcept {
  if constexpr (isAutoRef_v<T>)
    return t->toString();
  else
    return std::to_string(t);
}
template <> inline std::string toString(const std::string &t) noexcept { return t; }
