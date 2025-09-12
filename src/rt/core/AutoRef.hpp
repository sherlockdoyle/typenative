#pragma once

#include "Object.hpp"
#include "gc.hpp"
#include <atomic>
#include <cstddef>
#include <new>
#include <utility>

template <IsObject T> class WeakRef;

template <IsObject T> class AutoRef {
  Meta *meta = nullptr;
  T *obj = nullptr;

  AutoRef(T *obj) noexcept : meta(obj->meta), obj(obj) { meta->incRef(); }

  void safeIncRef() noexcept {
    if (meta)
      meta->incRef();
  }
  void safeDecRef() noexcept {
    std::atomic_thread_fence(std::memory_order_acquire);
    if (meta && meta->getRef() > 0 && meta->decRef() == 1) {
#ifndef NO_GC
      GC::gc().untrack(obj);
#endif
      delete obj;
      if (meta->decWeak() == 1)
        delete meta;
    }
  }

  friend class WeakRef<T>;
  friend class GC;

public:
  constexpr AutoRef() noexcept : meta(nullptr), obj(nullptr) {}
  constexpr AutoRef(std::nullptr_t) noexcept : meta(nullptr), obj(nullptr) {}

  AutoRef(const AutoRef &that) noexcept : meta(that.meta), obj(that.obj) { safeIncRef(); }
  AutoRef(AutoRef &&that) noexcept : meta(std::exchange(that.meta, nullptr)), obj(std::exchange(that.obj, nullptr)) {}

  ~AutoRef() noexcept { safeDecRef(); }

  AutoRef &operator=(std::nullptr_t) noexcept {
    safeDecRef();
    meta = nullptr;
    obj = nullptr;
    return *this;
  }
  AutoRef &operator=(T *thatObj) noexcept {
    if (!thatObj)
      return operator=(nullptr);

    if (obj != thatObj) {
      Meta *thatMeta = thatObj->meta;
      thatMeta->incRef();
      safeDecRef();
      meta = thatMeta;
      obj = thatObj;
    }
    return *this;
  }

  AutoRef &operator=(AutoRef &that) noexcept {
    if (obj != that.obj) {
      that.safeIncRef();
      safeDecRef();
      meta = that.meta;
      obj = that.obj;
    }
    return *this;
  }
  AutoRef &operator=(AutoRef &&that) noexcept {
    if (obj != that.obj) {
      safeDecRef();
      meta = std::exchange(that.meta, nullptr);
      obj = std::exchange(that.obj, nullptr);
    }
    return *this;
  }

  T &operator*() const noexcept { return *obj; }
  T *operator->() const noexcept { return obj; }
  explicit operator bool() const noexcept {
    // Ideally only checking for the object would be enough, but we do not want GC deleted objects to be accessible
    // (possibly from the destructor) while GC is running.
    return obj && meta->getRef(); // meta will be present if obj is present
  }

  std::size_t getRef() const noexcept { return meta ? meta->getRef() : 0; }
  std::size_t getWeak() const noexcept { return meta ? meta->getWeak() - 1 : 0; }

  template <typename... Args> static AutoRef make(Args &&...args) {
    return AutoRef(new T(std::forward<Args>(args)...));
  }
};