#pragma once

#include "../rt/AutoRef.hpp"
#include "../rt/Object.hpp"
#include "../util.hpp"
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T> class Array;
template <typename T> using $Array = AutoRef<Array<T>>;
template <typename T> class Array : virtual public Object {
  std::vector<T> data;

  void $forEachChild(std::function<void(Object *)> visitor) const noexcept override {
    if constexpr (isAutoRef_v<T>)
      for (auto t : data)
        visitor(t.operator->());
  }

  static std::size_t argSize(const T &) noexcept { return 1; }
  static std::size_t argSize(const $Array<T> &arr) noexcept { return arr->data.size(); }

  std::size_t normalizeIdx(const std::ptrdiff_t idx) const noexcept {
    std::ptrdiff_t len = data.size();
    if (idx < -len)
      return 0;
    if (idx < 0)
      return len + idx;
    if (idx < len)
      return idx;
    return len;
  }

  void append(const T &t) noexcept { data.push_back(t); }
  void append(const $Array<T> &arr) noexcept { data.insert(data.end(), arr->data.begin(), arr->data.end()); }

  template <typename U> friend class Array;

public:
  Array() noexcept {}
  Array(std::size_t size) noexcept { data.reserve(size); }
  Array(std::initializer_list<T> list) noexcept : data(list) {}

  static $Array<T> from(const $Array<T> &arr) noexcept {
    auto out = $Array<T>::make(arr->data.size());
    out->data.insert(out->data.end(), arr->data.begin(), arr->data.end());
    return out;
  }

  std::size_t length() const noexcept { return data.size(); }

  T operator[](const std::size_t i) noexcept { return data[i]; }
  T at(const std::ptrdiff_t i) const noexcept {
    if (i >= 0)
      return data[i];
    return data[data.size() + i];
  }

  std::size_t push(const $Array<T> &arr) noexcept {
    data.insert(data.end(), arr->data.begin(), arr->data.end());
    return data.size();
  }
  T pop() noexcept {
    T t = data.back();
    data.pop_back();
    return t;
  }

  T shift() noexcept {
    T t = data.front();
    data.erase(data.begin());
    return t;
  }
  std::size_t unshift(const $Array<T> &arr) noexcept {
    data.insert(data.begin(), arr->data.begin(), arr->data.end());
    return data.size();
  }

  // use template because we don't support unions yet
  template <typename... Args> $Array<T> concat(Args &&...args) const noexcept {
    static_assert((... && (std::convertible_to<std::remove_cvref_t<Args>, T> ||
                           std::same_as<std::remove_cvref_t<Args>, $Array<T>>)),
                  "All arguments must be of type T AutoRef<Array<T>>");

    std::size_t total = data.size();
    ((total += argSize(args)), ...);

    auto out = $Array<T>::make(total);
    out->data.insert(out->data.end(), data.begin(), data.end());
    (out->append(std::forward<Args>(args)), ...);

    return out;
  }

  $Array<T> slice(const std::ptrdiff_t start = 0) const noexcept { return slice(start, data.size()); }
  $Array<T> slice(const std::ptrdiff_t start, const std::ptrdiff_t end) const noexcept {
    std::size_t i = normalizeIdx(start), j = normalizeIdx(end);
    if (i >= j)
      return $Array<T>::make();

    auto out = $Array<T>::make();
    out->data.insert(out->data.end(), data.begin() + i, data.begin() + j);
    return out;
  }

  $Array<T> splice() noexcept { return $Array<T>::make(); }
  $Array<T> splice(const std::ptrdiff_t start = 0) noexcept { return splice(start, data.size()); }
  $Array<T> splice(const std::ptrdiff_t start, const std::size_t deleteCount,
                   const $Array<T> items = nullptr) noexcept {
    auto startIndex = normalizeIdx(start);
    auto deleteBegin = data.begin() + startIndex;
    auto deleteEnd = startIndex + deleteCount > data.size() ? data.end() : deleteBegin + deleteCount;

    auto out = $Array<T>::make();
    out->data.insert(out->data.end(), std::make_move_iterator(deleteBegin), std::make_move_iterator(deleteEnd));

    std::size_t actualDeleteCount = out->data.size();
    std::size_t insertCount = items ? items->data.size() : 0;

    if (insertCount < actualDeleteCount) { // array will get smaller
      auto newElementsEnd = items ? std::copy(items->data.begin(), items->data.end(), deleteBegin) : deleteBegin;
      auto tailStart = data.begin() + startIndex + actualDeleteCount;
      std::move(tailStart, data.end(), newElementsEnd);
      data.erase(data.end() - (actualDeleteCount - insertCount), data.end());
    } else if (insertCount > actualDeleteCount) { // array will get bigger
      std::size_t oldSize = data.size();
      data.resize(oldSize + (insertCount - actualDeleteCount));
      auto oldTailStart = data.begin() + startIndex + actualDeleteCount;
      auto oldTailEnd = data.begin() + oldSize;
      std::move_backward(oldTailStart, oldTailEnd, data.end());
      // we will be here only if insertCount > 0, in which case elementsToInsert is not null
      std::copy(items->data.begin(), items->data.end(), data.begin() + startIndex);
    } else if (items) {
      std::copy(items->data.begin(), items->data.end(), deleteBegin);
    }

    return out;
  }

  $Array<T> copyWithin(const std::ptrdiff_t target, const std::ptrdiff_t start) noexcept {
    return copyWithin(target, start, data.size());
  }
  $Array<T> copyWithin(const std::ptrdiff_t target, const std::ptrdiff_t start, const std::ptrdiff_t end) noexcept {
    std::size_t i = normalizeIdx(start), j = normalizeIdx(end);
    if (i >= j)
      return this;

    std::size_t k = normalizeIdx(target);
    std::size_t copyCount = std::min(j - i, data.size() - k);
    std::copy(data.begin() + i, data.begin() + i + copyCount, data.begin() + k);
    return this;
  }

  bool includes(const T &v) const noexcept { return std::find(data.begin(), data.end(), v) != data.end(); }
  std::ptrdiff_t indexOf(const T &v) const noexcept {
    auto it = std::find(data.begin(), data.end(), v);
    return it == data.end() ? -1 : std::distance(data.begin(), it);
  }
  std::ptrdiff_t lastIndexOf(const T &v) const noexcept {
    auto it = std::find(data.rbegin(), data.rend(), v);
    return it == data.rend() ? -1 : std::distance(data.rbegin(), it);
  }

  $Array<T> reverse() noexcept {
    std::reverse(data.begin(), data.end());
    return this;
  }

  $Array<T> sort() noexcept {
    std::sort(data.begin(), data.end());
    return this;
  }
  $Array<T> sort(const std::function<bool(const T &a, const T &b)> &cmp) noexcept {
    std::sort(data.begin(), data.end(), cmp);
    return this;
  }

  void forEach(const std::function<void(const T &, const std::size_t)> &f) const noexcept {
    std::size_t i = 0;
    for (const auto &t : data)
      f(t, i++);
  }

  template <typename U> $Array<U> map(const std::function<U(const T &, const std::size_t)> &f) const noexcept {
    auto out = $Array<U>::make(data.size());
    std::size_t i = 0;
    for (const auto &t : data)
      out->data.push_back(f(t, i++));
    return out;
  }

  $Array<T> filter(const std::function<bool(const T &, const std::size_t)> &f) const noexcept {
    auto out = $Array<T>::make();
    std::size_t i = 0;
    for (const auto &t : data)
      if (f(t, i++))
        out->data.push_back(t);
    return out;
  }

  T find(const std::function<bool(const T &, const std::size_t)> &f) const noexcept {
    std::size_t i = 0;
    for (const auto &t : data)
      if (f(t, i++))
        return t;

    return T{};
  }
  std::ptrdiff_t findIndex(const std::function<bool(const T &, const std::size_t)> &f) const noexcept {
    std::size_t i = 0;
    for (const auto &t : data) {
      if (f(t, i))
        return i;
      ++i;
    }

    return -1;
  }

  T findLast(const std::function<bool(const T &, const std::size_t)> &f) const noexcept {
    std::size_t i = data.size() - 1;
    for (auto it = data.rbegin(); it != data.rend(); ++it, --i)
      if (f(*it, i))
        return *it;

    return T{};
  }
  std::ptrdiff_t findLastIndex(const std::function<bool(const T &, const std::size_t)> &f) const noexcept {
    std::size_t i = data.size() - 1;
    for (auto it = data.rbegin(); it != data.rend(); ++it, --i)
      if (f(*it, i))
        return i;

    return -1;
  }

  T reduce(const std::function<T(const T &, const T &, const std::size_t)> &f) const noexcept {
    if (data.empty())
      return T{};

    std::size_t i = 0;
    T out = data[0];
    for (auto it = std::next(data.begin()); it != data.end(); ++it, ++i)
      out = f(out, *it, i);
    return out;
  }
  template <typename U = T>
  U reduce(const std::function<U(const U &, const T &, const std::size_t)> &f, const U &initial) const noexcept {
    std::size_t i = 0;
    U out = initial;
    for (const auto &t : data)
      out = f(out, t, i++);
    return out;
  }

  T reduceRight(const std::function<T(const T &, const T &, const std::size_t)> &f) const noexcept {
    if (data.empty())
      return T{};

    std::size_t i = data.size() - 1;
    T out = data[i];
    for (auto it = std::next(data.rbegin()); it != data.rend(); ++it, --i)
      out = f(out, *it, i);
    return out;
  }
  template <typename U = T>
  U reduceRight(const std::function<U(const U &, const T &, const std::size_t)> &f, const U &initial) const noexcept {
    std::size_t i = data.size() - 1;
    U out = initial;
    for (const auto &t : data)
      out = f(out, t, i--);
    return out;
  }

  bool some(const std::function<bool(const T &, const std::size_t)> &f) const noexcept {
    std::size_t i = 0;
    for (const auto &t : data)
      if (f(t, i++))
        return true;
    return false;
  }
  bool every(const std::function<bool(const T &, const std::size_t)> &f) const noexcept {
    std::size_t i = 0;
    for (const auto &t : data)
      if (!f(t, i++))
        return false;
    return true;
  }

  $Array<T> fill(const T &value, const std::ptrdiff_t start = 0) noexcept { return fill(value, start, data.size()); }
  $Array<T> fill(const T &value, const std::ptrdiff_t start, const std::ptrdiff_t end) noexcept {
    std::size_t i = normalizeIdx(start), j = normalizeIdx(end);
    for (; i < j; ++i)
      data[i] = value;
    return this;
  }

  $Array<T> toReversed() const noexcept {
    auto out = $Array<T>::make(data.size());
    out->data.insert(out->data.end(), data.rbegin(), data.rend());
    return out;
  }

  $Array<T> toSorted() const noexcept {
    auto out = $Array<T>::make();
    out->data = data;
    return out->sort();
  }
  $Array<T> toSorted(const std::function<bool(const T &, const T &)> &f) const noexcept {
    auto out = $Array<T>::make();
    out->data = data;
    return out->sort(f);
  }

  $Array<T> toSpliced(const std::ptrdiff_t start) const noexcept { return toSpliced(start, data.size()); }
  $Array<T> toSpliced(const std::ptrdiff_t start, const std::size_t skipCount,
                      const $Array<T> items = nullptr) const noexcept {
    std::size_t startIdx = normalizeIdx(start);
    std::size_t actualSkipCount = std::min(skipCount, data.size() - startIdx);
    auto out = $Array<T>::make(data.size() - actualSkipCount + (items ? items->data.size() : 0));

    out->data.insert(out->data.end(), data.begin(), data.begin() + startIdx);
    if (items)
      out->data.insert(out->data.end(), items->data.begin(), items->data.end());
    out->data.insert(out->data.end(), data.begin() + startIdx + actualSkipCount, data.end());

    return out;
  }

  $Array<T> with(const std::ptrdiff_t index, const T &value) const noexcept {
    auto out = $Array<T>::make();
    out->data = data;
    out->data[index >= 0 ? index : data.size() + index] = value;
    return out;
  }

  std::string join(const std::string &sep = ",") const noexcept {
    std::ostringstream oss;
    bool first = true;
    for (const auto &t : data) {
      if (first)
        first = false;
      else
        oss << sep;
      oss << ::toString(t);
    }
    return oss.str();
  }

  bool operator==(const $Array<T> &that) const noexcept { return data == that->data; }
  bool operator!=(const $Array<T> &that) const noexcept { return data != that->data; }

  std::string toString() const noexcept override {
    std::ostringstream oss;
    oss << "[";
    bool first = true;

    for (auto &t : data) {
      if (first)
        first = false;
      else
        oss << ", ";

      oss << ::toString(t);
    }

    oss << "]";
    return oss.str();
  }
};