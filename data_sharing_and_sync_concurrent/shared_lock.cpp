#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>

class A {
 public:
  void lock_shared() { std::cout << "lock_shared" << std::endl; }
  void unlock_shared() { std::cout << "unlock_shared" << std::endl; }
};

class B {
 public:
  int read() const {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::shared_lock<std::shared_mutex> l(m_);
    return n_;
  }

  int write() {
    std::unique_lock<std::shared_mutex> l(m_);
    return ++n_;
  }

 private:
  mutable std::shared_mutex m_;
  int n_ = 0;
};

void read(const B& b) { std::cout << b.read() << std::endl; }

void write(B& b) { b.write(); }

int main() {
  A a;
  {
    std::shared_lock l(a);  // lock_shared
  }                         // unlock_shared

  {
    B b;
    std::thread th1(read, std::ref(b));
    std::thread th2(write, std::ref(b));
    th1.join();
    th2.join();
  }
}