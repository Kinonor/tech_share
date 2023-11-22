#include <iostream>
#include <latch>
#include <string>
#include <thread>

class A {
 public:
  void f() {
    for (auto& x : data_) {
      x.t = std::jthread([&] {
        x.s += x.s;
        done_.count_down();
      });
    }
    done_.wait();  // 阻塞至计数为0，保证所有的线程执行完
    for (auto& x : data_) {
      std::cout << x.s << std::endl;
    }
  }

 private:
  struct {
    std::string s;
    std::jthread t;
  } data_[3] = {
      {"hello"},
      {"down"},
      {"demo"},
  };

  std::latch done_{3};
};

int main() {
  A a;
  a.f();
}