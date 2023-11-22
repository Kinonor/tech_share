#include <exception>
#include <iostream>
#include <thread>   // std::thread
#include <utility>  // std::forward/move
#include <vector>

class scoped_thread {
 public:
  template <typename... Arg>
  scoped_thread(Arg&&... arg) noexcept : thread_(std::forward<Arg>(arg)...) {
    std::cout << "move" << std::endl;
  }
  scoped_thread(scoped_thread&& other) = default;
  scoped_thread& operator=(scoped_thread&& other) = default;
  ~scoped_thread() {
    if (thread_.joinable()) {
      thread_.join();
    }
  }

 private:
  std::thread thread_;
  // std::thread& ？ 可以会导致多处使用。
};

void func() { std::cout << "hello" << std::endl; }
void func2(int i) { std::cout << i << std::endl; }

int main() {
  std::thread th_0(func2, 1);
  scoped_thread thread_guard(std::move(th_0));

  std::thread th(func);
  scoped_thread thread_guard_2 = std::move(th);

  scoped_thread thread_guard_3([]() { std::cout << "thread 3" << std::endl; });

  std::vector<scoped_thread> list;
  list.push_back([]() {});
  list.push_back([]() {});

  return 0;
}