#include "src/core/rt/AutoRef.hpp"
#include "src/core/rt/Object.hpp"
#include "src/core/rt/WeakRef.hpp"
#include "src/core/rt/gc.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <ostream>
#include <random>
#include <string>
#include <thread>

class Node : virtual public Object {
public:
  std::string name;
  AutoRef<Node> next, child;

  Node(const std::string &n) : name(n) {
    // std::cout << "Constructing Node " << name << std::endl;
  }
  ~Node() {
    // std::cout << "Destroying Node " << name << std::endl;
  }

  REGISTER_CHILDREN(next, child)
};

class WNode : virtual public Object {
public:
  std::string name;
  AutoRef<WNode> strong_child;
  WeakRef<WNode> weak_child;

  WNode(const std::string &n) : name(n) { std::cout << "Construct " << name << std::endl; }
  ~WNode() override { std::cout << "Destroy " << name << std::endl; }

  REGISTER_CHILDREN(strong_child)
};

class FinalNode : virtual public Object {
public:
  std::string name;
  WeakRef<FinalNode> other;

  FinalNode(const std::string &n) : name(n) { std::cout << "Construct " << name << std::endl; }
  ~FinalNode() override {
    std::cout << "Destroying " << name << " (in ~FinalNode). Trying to lock other..." << std::endl;
    if (other) {
      auto o = other.lock();
      if (o)
        std::cout << "~" << name << " sees other: " << o->name << std::endl;
      else
        std::cout << "~" << name << " other expired" << std::endl;
    } else {
      std::cout << "~" << name << " has no weak other" << std::endl;
    }
  }
};

class TouchNode : virtual public Object {
public:
  std::string name;
  AutoRef<TouchNode> other;

  TouchNode(const std::string &n) : name(n) { std::cout << "Construct " << name << std::endl; }
  ~TouchNode() override {
    std::cout << "~" << name << " running; will access other." << std::endl;
    if (other)
      std::cout << name << " sees " << other->name << std::endl;
    else
      std::cout << name << " sees nothing" << std::endl;
  }

  REGISTER_CHILDREN(other)
};

void weakLocker(WeakRef<Node> w, std::atomic_bool &stop) {
  while (!stop)
    auto locked = w.lock();
}

int main() {
  {
    std::cout << "=== Basic ownership test ===" << std::endl;
    auto a = AutoRef<Node>::make("A");
    std::cout << "A ref: " << a.getRef() << ", weak: " << a.getWeak() << std::endl;
    {
      AutoRef<Node> a2 = a;
      std::cout << "A after copy ref: " << a.getRef() << ", weak: " << a.getWeak() << std::endl;
    }
    std::cout << "A after inner scope ref: " << a.getRef() << ", weak: " << a.getWeak() << std::endl;
  }

  std::cout << std::endl;
  WeakRef<Node> w;
  {
    std::cout << "=== WeakRef expiration test ===" << std::endl;
    auto b = AutoRef<Node>::make("B");
    std::cout << "B ref: " << b.getRef() << ", weak: " << b.getWeak() << std::endl;
    w = b;
    std::cout << "B after weak ref: " << b.getRef() << ", weak: " << b.getWeak() << std::endl;
    std::cout << "Weak(B) is " << (w ? "alive" : "dead") << std::endl;
  }
  std::cout << "Weak(B) outside scope is " << (w ? "alive" : "dead") << std::endl;

  std::cout << std::endl;
  {
    std::cout << "=== Cycle without GC ===" << std::endl;
    auto a = AutoRef<Node>::make("A");
    auto b = AutoRef<Node>::make("B");
    a->next = b;
    b->next = a;
    std::cout << "A ref: " << a.getRef() << ", weak: " << a.getWeak() << std::endl;
    std::cout << "B ref: " << b.getRef() << ", weak: " << b.getWeak() << std::endl;

    w = b;
    std::cout << "B after weak ref: " << b.getRef() << ", weak: " << b.getWeak() << std::endl;
    std::cout << "Weak(B) is " << (w ? "alive" : "dead") << std::endl;
  }
  std::cout << "Weak(B) outside scope is " << (w ? "alive" : "dead") << std::endl;
  // GC::gc().collect();
  std::cout << "Weak(B) after GC is " << (w ? "alive" : "dead") << std::endl;

  std::cout << std::endl;
  {
    std::cout << "=== Mixed scenario ===" << std::endl;
    auto a = AutoRef<Node>::make("A");
    auto b = AutoRef<Node>::make("B");
    a->child = b;
    b->next = a;
    b->child = AutoRef<Node>::make("C");
    std::cout << "A ref: " << a.getRef() << ", weak: " << a.getWeak() << std::endl;
    std::cout << "B ref: " << b.getRef() << ", weak: " << b.getWeak() << std::endl;
    std::cout << "C ref: " << b->child.getRef() << ", weak: " << b->child.getWeak() << std::endl;
  }
  // GC::gc().collect();

  std::cout << std::endl;
  {
    std::cout << "=== Move semantics test ===" << std::endl;
    auto m1 = AutoRef<Node>::make("M1");
    std::cout << "M1 ref: " << m1.getRef() << ", weak: " << m1.getWeak() << std::endl;
    auto m2 = std::move(m1);
    std::cout << "After move M2 ref: " << m2.getRef() << ", weak: " << m2.getWeak() << std::endl;
    std::cout << "M1 is " << (m1 ? "alive" : "dead") << std::endl;
  }

  std::cout << std::endl;
  {
    std::cout << "=== Many WeakRefs test ===" << std::endl;
    WeakRef<Node> w1, w2, w3;
    {
      auto x = AutoRef<Node>::make("X");
      w1 = x;
      w2 = x;
      w3 = w1;
      std::cout << "X ref: " << x.getRef() << ", weak: " << x.getWeak() << std::endl;
    }
    std::cout << "Weak1(X) after scope is " << (w1 ? "alive" : "dead") << std::endl;
    std::cout << "Weak2(X) after scope is " << (w2 ? "alive" : "dead") << std::endl;
    std::cout << "Weak3(X) after scope is " << (w3 ? "alive" : "dead") << std::endl;
  }

  std::cout << std::endl;
  {
    std::cout << "=== Self-cycle test ===" << std::endl;
    auto s = AutoRef<Node>::make("Self");
    s->next = s;
    std::cout << "Self ref: " << s.getRef() << ", weak: " << s.getWeak() << std::endl;
    w = s;
  }
  std::cout << "Weak(Self) after scope is " << (w ? "alive" : "dead") << std::endl;
  // GC::gc().collect();
  std::cout << "Weak(Self) after GC is " << (w ? "alive" : "dead") << std::endl;

  std::cout << std::endl;
  WeakRef<WNode> w2;
  {
    std::cout << "=== Weak member vs strong ===" << std::endl;
    auto p = AutoRef<WNode>::make("Parent");
    auto c = AutoRef<WNode>::make("Child");
    p->strong_child = c;
    p->weak_child = c;
    w2 = c;
    std::cout << "Child ref: " << c.getRef() << ", weak: " << c.getWeak() << std::endl;
  }
  std::cout << "Weak(Child) after scope is " << (w2 ? "alive" : "dead") << std::endl;
  // GC::gc().collect();
  std::cout << "Weak(Child) after GC is " << (w2 ? "alive" : "dead") << std::endl;

  std::cout << std::endl;
  {
    std::cout << "=== Destructor locking WeakRef ===" << std::endl;
    auto a = AutoRef<FinalNode>::make("A");
    auto b = AutoRef<FinalNode>::make("B");
    a->other = b;
    b->other = a;
    std::cout << "A ref: " << a.getRef() << ", weak: " << a.getWeak() << std::endl;
    std::cout << "B ref: " << b.getRef() << ", weak: " << b.getWeak() << std::endl;
  }
  // GC::gc().collect();

  std::cout << std::endl;
  {
    std::cout << "=== Long chain test ===" << std::endl;
    auto head = AutoRef<Node>::make("Chain0");
    auto cur = head;
    for (int i = 1; i < 20; ++i) {
      auto next = AutoRef<Node>::make("Chain" + std::to_string(i));
      cur->next = next;
      cur = next;
    }
    cur->next = head;
  }
  // GC::gc().collect();

  std::cout << std::endl;
  {
    std::cout << "=== Multiple roots test ===" << std::endl;
    auto root1 = AutoRef<Node>::make("Root1");
    auto root2 = AutoRef<Node>::make("Root2");
    auto shared = AutoRef<Node>::make("Shared");
    root1->child = shared;
    root2->child = shared;
    w = shared;
    std::cout << "Shared ref: " << shared.getRef() << ", weak: " << shared.getWeak() << std::endl;
  }
  std::cout << "Weak(Shared) after scope is " << (w ? "alive" : "dead") << std::endl;

  std::cout << std::endl;
  {
    std::cout << "=== Repeated GC test ===" << std::endl;
    // GC::gc().collect();
    // GC::gc().collect();
    auto a = AutoRef<Node>::make("A");
    // GC::gc().collect();
    // GC::gc().collect();
  }

  std::cout << std::endl;
  int N = 8765, S = 98;
  {
    std::cout << "=== Stress random graph test ===" << std::endl;
    std::vector<AutoRef<Node>> nodes;
    for (int i = 0; i < N; ++i)
      nodes.push_back(AutoRef<Node>::make("N" + std::to_string(i)));

    std::mt19937 rng(S);
    std::uniform_int_distribution<int> idx(0, nodes.size() - 1);

    for (int i = 0, l = nodes.size() * (nodes.size() - 1); i < l; ++i) {
      int a = idx(rng), b = idx(rng);
      nodes[a]->next = nodes[b];
    }
  }
  // auto n = GC::gc().collect();
  // std::cout << "Created " << N << " object graph with seed " << S << ". Collected " << n << " objects." <<
  // std::endl;

  std::cout << std::endl;
  {
    std::cout << "=== Destructor access other test ===" << std::endl;
    auto a = AutoRef<TouchNode>::make("A");
    auto b = AutoRef<TouchNode>::make("B");
    a->other = b;
    b->other = a;
    std::cout << "A ref: " << a.getRef() << ", weak: " << a.getWeak() << std::endl;
    std::cout << "B ref: " << b.getRef() << ", weak: " << b.getWeak() << std::endl;
  }
  // GC::gc().collect();

  std::cout << std::endl;
  {
    std::cout << "=== Concurrent lock test ===" << std::endl;
    std::atomic_bool stopFlag(false);
    WeakRef<Node> w;
    std::thread t1;
    {
      auto strong = AutoRef<Node>::make("Concurrent");
      w = strong;
      t1 = std::thread(weakLocker, w, std::ref(stopFlag));
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stopFlag = true;
    t1.join();
    std::cout << "Weak(Concurrent) after scope is " << (w ? "alive" : "dead") << std::endl;
  }

  return 0;
}