#include <chrono>
#include <future>
#include <iostream>

class A {
 public:
  void task() { std::cout << 1 << std::endl; }
  void wait_for_task() {
    ps_.get_future().wait();
    task();
  }
  void signal() { ps_.set_value(); }

 private:
  std::promise<void> ps_;  // 只是通知作用，返回值为 void
};

void task() { std::cout << 1; }

int main() {
  A a;
  std::thread t(&A::wait_for_task, &a);
  a.signal();
  t.join();
}