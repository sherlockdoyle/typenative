#pragma once

#include "AutoRef.hpp"
#include "Object.hpp"
#include <cstddef>

template <IsObject T> class WeakRef {
  Meta *meta = nullptr;
  T *obj = nullptr;

  void safeIncRef() noexcept {
    if (meta)
      meta->incWeak();
  }
  void safeDecRef() noexcept {
    if (meta && meta->decWeak() == 1)
      delete meta;
  }

public:
  constexpr WeakRef() noexcept : meta(nullptr), obj(nullptr) {}
  constexpr WeakRef(std::nullptr_t) noexcept : meta(nullptr), obj(nullptr) {}

  WeakRef(const AutoRef<T> &ref) noexcept : meta(ref.meta), obj(ref.obj) { safeIncRef(); }

  WeakRef(const WeakRef &that) noexcept : meta(that.meta), obj(that.obj) { safeIncRef(); }
  WeakRef(WeakRef &&that) noexcept : meta(std::exchange(that.meta, nullptr)), obj(std::exchange(that.obj, nullptr)) {}

  ~WeakRef() noexcept { safeDecRef(); }

  WeakRef &operator=(std::nullptr_t) noexcept {
    safeDecRef();
    meta = nullptr;
    obj = nullptr;
    return *this;
  }

  WeakRef &operator=(const AutoRef<T> &ref) noexcept {
    Meta *thatMeta = ref.meta;
    if (!thatMeta)
      return operator=(nullptr);

    if (meta != thatMeta) {
      thatMeta->incWeak();
      safeDecRef();
      meta = thatMeta;
      obj = ref.obj;
    }
    return *this;
  }

  WeakRef &operator=(WeakRef &that) noexcept {
    if (meta != that.meta) {
      that.safeIncRef();
      safeDecRef();
      meta = that.meta;
      obj = that.obj;
    }
    return *this;
  }
  WeakRef &operator=(WeakRef &&that) noexcept {
    if (meta != that.meta) {
      safeDecRef();
      meta = std::exchange(that.meta, nullptr);
      obj = std::exchange(that.obj, nullptr);
    }
    return *this;
  }

  explicit operator bool() const noexcept {
    return obj && meta->getRef(); // meta will be present if obj is present
  }

  AutoRef<T> lock() noexcept {
    AutoRef<T> ret;
    if (obj) {
      std::size_t ref = meta->getRef();
      while (ref)
        if (meta->ref.compare_exchange_weak(ref, ref + 1, std::memory_order_acq_rel, std::memory_order_acquire)) {
          ret.meta = meta; // assign manually to avoid double inc
          ret.obj = obj;
          return ret;
        }
      obj = nullptr;
    }
    return ret;
  }
};