#pragma once

#include "../rt/AutoRef.hpp"
#include "../rt/Object.hpp"
#include "array.hpp"
#include "valTypes.hpp"
#include <algorithm>
#include <cstddef>
#include <string>

class String;
using $String = AutoRef<String>;
class String : virtual public Object {
  std::string str;

public:
  String() noexcept {}
  String(std::string str) : str(str) {}

  std::size_t length() const noexcept { return str.length(); }

  $String at(std::size_t idx) const noexcept { return $String::make(str.substr(idx, 1)); }

  $Array<$String> split(const $String &sep) {
    auto out = $Array<$String>::make();
    size_t start = 0, end = str.find(sep->str);

    while (end != std::string::npos) {
      out->push($String::make(str.substr(start, end - start)));
      start = end + sep->str.length();
      end = str.find(sep->str, start);
    }
    out->push($String::make(str.substr(start)));

    return out;
  }

  $String substring(const std::size_t start, const std::size_t end) const noexcept {
    std::size_t i = std::max(0ul, start), j = std::min(end, str.length());
    return $String::make(str.substr(i, j - i));
  }

  $String add(const $String &that) const noexcept { return $String::make(str + that->str); }

  bool isInt() const noexcept {
    try {
      std::stoi(str);
      return true;
    } catch (...) {
      return false;
    }
  }
  bool isFloat() const noexcept {
    try {
      std::stod(str);
      return true;
    } catch (...) {
      return false;
    }
  }
  bool isAlpha() const noexcept {
    for (char c : str)
      if (!std::isalpha(c))
        return false;
    return true;
  }
  bool isAlNum() const noexcept {
    for (char c : str)
      if (!std::isalnum(c))
        return false;
    return true;
  }

  bool operator==(const $String &that) const noexcept { return str == that->str; }
  bool operator!=(const $String &that) const noexcept { return str != that->str; }

  std::string _str() const noexcept { return str; }
  std::string toString() const noexcept override { return str; }
};

inline $String newString(std::string str) { return $String::make(str); }
template <typename T> $String StringFrom(T t) { return $String::make(std::to_string(t)); }

inline i64 parseInt(const $String &str) { return std::stoi(str->_str()); }
inline f64 parseFloat(const $String &str) { return std::stod(str->_str()); }