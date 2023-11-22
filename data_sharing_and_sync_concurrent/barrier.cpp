#include <barrier>
#include <cassert>
#include <iostream>
#include <thread>

class A {
 public:
  void f() {
    std::barrier sync_point{3, [&]() noexcept { ++i_; }};
    for (auto& x : tasks_) {
      x = std::thread([&] {
        std::cout << 1 << std::endl;
        sync_point.arrive_and_wait();  // 所有的线程都到达次同步点后，再往下执行
        assert(i_ == 1);
        std::cout << 2 << std::endl;
        sync_point.arrive_and_wait();  // 所有的线程都到达次同步点后，再往下执行
        assert(i_ == 2);
        std::cout << 3 << std::endl;
      });
    }
    for (auto& x : tasks_) {
      x.join();
    }
  }

 private:
  std::thread tasks_[3] = {};
  int i_ = 0;
};

int main() {
  A a;
  a.f();
}