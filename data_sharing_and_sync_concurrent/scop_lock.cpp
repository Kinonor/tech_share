#include <iostream>
#include <mutex>

class A {
 public:
  void lock() { std::cout << 1 << std::endl; }
  void unlock() { std::cout << 2 << std::endl; }
  bool try_lock() {
    std::cout << 3;
    return true;
  }
};

class B {
 public:
  void lock() { std::cout << 4 << std::endl; }
  void unlock() { std::cout << 5 << std::endl; }
  bool try_lock() {
    std::cout << 6 << std::endl;
    return true;
  }
};

int main() {
  A a;
  B b;
  {
    std::scoped_lock l(a, b);
    std::cout << std::endl;
  }
}