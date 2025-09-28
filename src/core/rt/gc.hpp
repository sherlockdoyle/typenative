#pragma once

#include "AutoRef.hpp"
#include "Object.hpp"
#include "gcStat.hpp"
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_set>
#include <vector>

class GC {
  enum State { IDLE, COLLECTING, PAUSED };

  std::unordered_set<Object *> youngTracked, oldTracked;
  GCStat stats;
  std::mutex mtx;
  State state = IDLE; // or use a mutex?
  std::uint8_t objCount = 0;

  ~GC() {
    std::vector<Meta *> metas;
    metas.reserve(youngTracked.size() + oldTracked.size());
    for (Object *obj : youngTracked) {
      obj->meta->zeroRef();
      metas.push_back(obj->meta);
    }
    for (Object *obj : oldTracked) {
      obj->meta->zeroRef();
      metas.push_back(obj->meta);
    }

    for (Object *obj : youngTracked)
      delete obj;
    for (Object *obj : oldTracked)
      delete obj;
    for (Meta *meta : metas)
      delete meta;
  }

  void track(Object *obj) {
    // try collecting first
    if (++objCount == 255) { // objCount wraps around, so we check once every 256 allocations
      if (stats.shouldDoYoungGC(youngTracked.size()))
        collect(false);
      else if (stats.shouldDoOldGC(oldTracked.size()))
        collect(true);
    }

    std::lock_guard<std::mutex> lk(mtx);
    youngTracked.insert(obj);
  }

  void untrack(Object *obj) {
    std::lock_guard<std::mutex> lk(mtx);
    if (!youngTracked.erase(obj))
      oldTracked.erase(obj);
  }

  std::size_t collect(const bool old = false) {
    std::vector<Object *> outRefs; // stack
    {
      std::lock_guard<std::mutex> lk(mtx); // only one collection at a time
      std::unordered_set<Object *> &tracked = old ? oldTracked : youngTracked;
      if (state != IDLE || tracked.empty())
        return 0;
      state = COLLECTING;

      // detect objects which have refs from outside (variables)
      for (Object *obj : tracked) // copy ref
        obj->meta->copyRef();
      for (Object *obj : tracked)
        obj->$forEachChild([](Object *child) {
          if (child)
            --child->meta->outRef;
        });

      // dfs to find all objects with transitive refs from outside
      outRefs.reserve(tracked.size());
      for (Object *obj : tracked)
        if (obj->meta->outRef > 0) // has refs from outside
          outRefs.push_back(obj);
      while (!outRefs.empty()) {
        Object *obj = outRefs.back();
        outRefs.pop_back();
        obj->$forEachChild([&outRefs](Object *child) {
          if (child && child->meta->outRef == 0) {
            ++child->meta->outRef; // mark as visited
            outRefs.push_back(child);
          }
        });
      }

      // remove objects with no refs from outside
      for (auto it = tracked.begin(); it != tracked.end();)
        if ((*it)->meta->outRef == 0) { // no refs from outside
          outRefs.push_back(*it);       // reuse stack
          (*it)->meta->zeroRef();
          it = tracked.erase(it);
        } else
          ++it;

      if (old)
        stats.updateOld(oldTracked.size());
      else { // young
        stats.updateYoung(youngTracked.size());
        // move all leftover young objects to old
        oldTracked.insert(youngTracked.begin(), youngTracked.end());
        youngTracked.clear();
      }
    }

    // delete unreferenced objects
    std::vector<Meta *> metas;
    metas.reserve(outRefs.size());
    for (Object *obj : outRefs) {
      metas.push_back(obj->meta);
      delete obj; // TODO: two collect call can run this stage (outside of lock) simultaneously, will there be double
                  // free?
    }
    for (Meta *meta : metas)
      if (meta->decWeak() == 1)
        delete meta;

    if (state == COLLECTING)
      state = IDLE;
    return outRefs.size();
  }

  template <typename T> friend class AutoRef;

public:
  static GC &gc() {
    static GC gc;
    return gc;
  }

  void pause() { state = PAUSED; }
  void resume() { state = IDLE; }

  void forceCollect(bool old = false) { collect(old); }
};