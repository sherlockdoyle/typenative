### A Hybrid Generational Garbage Collector with Reference Counting and Cycle Detection

#### Abstract

This document describes a hybrid, generational garbage collection (GC) system designed for C++ environments. The system integrates atomic reference counting for efficient reclamation of non-cyclic garbage with a mark-sweep based cycle collector to manage complex object graphs. Memory is divided into young and old generations to optimize collection frequency based on the generational hypothesis. The design prioritizes thread safety for smart pointer operations while ensuring collection phases are serialized to maintain correctness.

-----

#### 1. Core Principles and Components

The memory management strategy is built upon three primary components:

1.  **`Object` and `Meta`:** All managed objects must inherit from a common `Object` base class. Each `Object` is associated with a `Meta` control block that stores two atomic reference counts: a **strong count** (`ref`) and a **weak count** (`weak`).
2.  **`AutoRef<T>`:** A smart pointer that implements shared ownership via strong references. It guarantees that an object will remain alive as long as at least one `AutoRef` points to it. The lifecycle of `AutoRef` instances directly manipulates the atomic `ref` count.
3.  **`WeakRef<T>`:** A non-owning smart pointer that observes an object without extending its lifetime. It holds a weak reference, manipulating the `weak` count. A `WeakRef` can be temporarily and safely "locked" to create a new `AutoRef`, preventing the object from being deleted during access.

-----

#### 2. Memory Management Strategy

The system employs a dual-pronged approach:

##### 2.1. Reference Counting

For the majority of objects, lifetime is managed by simple reference counting.

  * **Allocation:** An `AutoRef` is created, and the object's strong reference count (`ref`) is initialized to 1.
  * **Copying:** Copying an `AutoRef` atomically increments the `ref` count.
  * **Destruction:** Destroying an `AutoRef` atomically decrements the `ref` count.
  * **Deallocation:** When the `ref` count drops to zero, the object has no more strong references and is immediately deleted. The `Meta` block persists until the `weak` count also drops to zero.

This method is highly efficient for acyclic data structures but fails to reclaim objects involved in reference cycles.

##### 2.2. Cycle Collection

To handle reference cycles, a tracing collector is periodically invoked. The algorithm is a variation of mark-sweep tailored for a reference-counted environment.

1.  **Pause:** The collector first acquires a global mutex, pausing all new allocations and subsequent collections. This creates a consistent snapshot of the object graph.

2.  **Root Identification (The `outRef` Calculation):** The collector must identify objects that are reachable from outside the managed heap (i.e., from local variables on the stack).

      * It first iterates through all tracked objects, initializing a temporary, non-atomic counter (`outRef`) in each `Meta` block with the value of the current strong `ref` count.
      * It then performs a second iteration over all objects. For each object, it traverses its children (objects it holds an `AutoRef` to) and decrements their respective `outRef` counters.
      * After this phase, any object with an `outRef > 0` is considered a **root**, as it is referenced by at least one `AutoRef` that is not itself part of the tracked heap (e.g., a variable on the stack).

3.  **Marking (Graph Traversal):** Starting from the identified roots, the collector performs a depth-first search (DFS) traversal of the object graph. Every reachable object is marked as "live." In this implementation, the "mark" is achieved by re-purposing the `outRef` field; any object visited during the traversal has its `outRef` value modified, distinguishing it from unvisited objects whose `outRef` remains zero.

4.  **Sweeping (Reclamation):** The collector iterates through the set of all tracked objects.

      * Any object not marked as "live" (i.e., its `outRef` is still zero) is unreachable garbage and is condemned.
      * Before deallocation, the `ref` count of all condemned objects is atomically set to zero. This "zeroing" acts as a signal to any concurrent `AutoRef` destructors that the GC is handling deletion, thus preventing double-frees.
      * Finally, the condemned objects are deleted, and their associated `Meta` blocks are cleaned up if no `WeakRef`s remain.

-----

#### 3. Generational Collection

To optimize performance, the collector is generational, based on the hypothesis that most objects die young.

  * **Generations:** The heap is divided into a **Young Generation** (`youngTracked`) and an **Old Generation** (`oldTracked`).
  * **Allocation:** All new objects are allocated in the Young Generation.
  * **Minor Collection:** The GC preferentially collects the Young Generation. This is faster as the set of objects is much smaller.
  * **Promotion:** All objects that survive a young generation collection are promoted to the Old Generation.
  * **Major Collection:** The Old Generation is collected far less frequently, typically when its size exceeds a much larger threshold.
  * **Adaptive Thresholds:** The decision to trigger a collection is managed by an `AdaptiveEstimator`, which dynamically adjusts the size thresholds for both generations based on the amount of garbage reclaimed in previous cycles. This heuristic aims to maximize throughput by collecting only when it is likely to be productive.

-----

#### 4. Concurrency Model

  * **Atomic Operations:** All modifications to reference counts are performed using `std::atomic` with appropriate memory ordering (`relaxed` for increments, `acq_rel` for decrements) to ensure visibility and prevent race conditions during smart pointer operations.
  * **Collection Serialization:** The entire collection process is serialized by a single `std::mutex`. This "Stop-the-World" approach simplifies the algorithm but introduces latency.
  * **Safe Weak-to-Strong Promotion:** The `WeakRef::lock()` method provides a thread-safe mechanism to upgrade a weak reference to a strong `AutoRef` using an atomic `compare_exchange` loop, preventing data races when accessing potentially expired objects.