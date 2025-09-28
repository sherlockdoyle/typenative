#pragma once

#include <atomic>
#include <cstddef>
#include <functional>

struct Meta {
  std::atomic_size_t ref{0}, weak{1};
  std::size_t outRef; // number of refs from outside the GC (variables)

  size_t getRef() const { return ref.load(std::memory_order_relaxed); } // relaxed or acquire?
  void incRef() { ref.fetch_add(1, std::memory_order_relaxed); }
  size_t decRef() { return ref.fetch_sub(1, std::memory_order_acq_rel); }
  void zeroRef() { ref.store(0, std::memory_order_relaxed); }

  size_t getWeak() const { return weak.load(std::memory_order_relaxed); }
  void incWeak() { weak.fetch_add(1, std::memory_order_relaxed); }
  size_t decWeak() { return weak.fetch_sub(1, std::memory_order_acq_rel); }

  void copyRef() { outRef = getRef(); }

private:
  enum { MARKED_SHIFT = 1 << 1 };
};

class Object {
  Meta *meta;

  Object(Object &&) = delete;
  Object(const Object &) = delete;
  Object &operator=(Object &&) = delete;
  Object &operator=(const Object &) = delete;

  template <typename T> friend class AutoRef;
  friend class GC;

public:
  Object() : meta(new Meta()) {}
  virtual ~Object() {}

  template <typename T> bool operator==(const T &that) const { return this == that; }
  template <typename T> bool operator!=(const T &that) const { return this != that; }

  virtual std::string toString() const { return typeid(*this).name(); }

  virtual void $forEachChild(std::function<void(Object *)> visitor) const noexcept {}
};

// Recipe taken from https://www.scs.stanford.edu/~dm/blog/va-opt.html
#define PARENS ()
#define EXPAND4(...) __VA_ARGS__
#define EXPAND3(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND2(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND1(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))

#define FOR_EACH(macro, ...) __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define FOR_EACH_HELPER(macro, a1, ...) macro(a1) __VA_OPT__(FOR_EACH_AGAIN PARENS(macro, __VA_ARGS__))
#define FOR_EACH_AGAIN() FOR_EACH_HELPER

// we (ab)use the arrow operator to get the object!
#define VISITOR_CALL(m) visitor(this->m.operator->());
#define REGISTER_CHILDREN(...)                                                                                         \
  void $forEachChild(std::function<void(Object *)> visitor) const noexcept override {                                  \
    FOR_EACH(VISITOR_CALL, __VA_ARGS__)                                                                                \
  }
