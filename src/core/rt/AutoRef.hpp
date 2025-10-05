#pragma once

#include "Object.hpp"
#include "gc.hpp"
#include <atomic>
#include <cstddef>
#include <new>
#include <utility>

template <typename T> class WeakRef;

template <typename T> class AutoRef {
  Meta *meta = nullptr;
  T *obj = nullptr;

  void safeIncRef() const noexcept {
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

public:
  constexpr AutoRef() noexcept : meta(nullptr), obj(nullptr) {}
  constexpr AutoRef(std::nullptr_t) noexcept : meta(nullptr), obj(nullptr) {}
  AutoRef(T *obj) noexcept : meta(obj->meta), obj(obj) { meta->incRef(); }

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

  AutoRef &operator=(const AutoRef &that) noexcept {
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

  template <typename U> bool operator==(const AutoRef<U> &that) const noexcept { return obj == that.obj; }
  template <typename U> bool operator==(const U *that) const noexcept { return obj == that; }
  template <typename U> friend bool operator==(const U *that, const AutoRef<T> &ref) { return that == ref.obj; }
  template <typename U> bool operator!=(const AutoRef<U> &that) const noexcept { return obj != that.obj; }
  template <typename U> bool operator!=(const U *that) const noexcept { return obj != that; }
  template <typename U> friend bool operator!=(const U *that, const AutoRef<T> &ref) { return that != ref.obj; }
#define OP(op)                                                                                                         \
  template <typename U> bool operator op(const AutoRef<U> &that) const noexcept { return (*obj)op(*that.obj); }        \
  template <typename U> bool operator op(const U *that) const noexcept { return (*obj)op(*that); }                     \
  template <typename U> friend bool operator op(const U *that, const AutoRef<T> &ref) { return (*that)op(*ref.obj); }
  OP(<)
  OP(>)
  OP(<=)
  OP(>=)
#undef OP

  std::size_t getRef() const noexcept { return meta ? meta->getRef() : 0; }
  std::size_t getWeak() const noexcept { return meta ? meta->getWeak() - 1 : 0; }

  template <typename U> AutoRef<U> as() const noexcept { return AutoRef<U>(dynamic_cast<U *>(obj)); }

  template <typename... Args> static AutoRef make(Args &&...args) {
    T *obj = new T(std::forward<Args>(args)...);
#ifndef NO_GC
    GC::gc().track(obj);
#endif
    return AutoRef(obj);
  }

  template <typename... Args> static AutoRef makeNoGC(Args &&...args) {
    return AutoRef(new T(std::forward<Args>(args)...));
  }
};

template <typename> struct isAutoRef : std::false_type {};
template <typename T> struct isAutoRef<AutoRef<T>> : std::true_type {};
template <typename T> constexpr bool isAutoRef_v = isAutoRef<T>::value;