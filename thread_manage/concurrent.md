# C++ 并编程
## 线程管理
### 如何写一个线程管理

```C++
#include <iostream>
#include <thread>   // std::thread
#include <utility>  // std::forward/move

class scoped_thread {
 public:
  template <typename... Arg>
  scoped_thread(Arg&&... arg) : thread_(std::forward<Arg>(arg)...) {}
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
  std::thread th_0(func2);
  scoped_thread thread_guard(std::move(th_0), 2);

  std::thread th(func);
  scoped_thread thread_guard_2 = std::move(th);

  return 0;
}

```
### 线程所有权

```C++
#include <iostream>
#include <thread>   // std::thread
#include <utility>  // std::forward/move

void func() { std::cout << std::this_thread::get_id() << std::endl; }

int main() {
  std::thread th0(func);

  std::thread th2 = std::move(th0);
  if (th2.joinable()) {
    std::cout << "is th2<<<<<<<<<" << std::endl;
    th2.join();
  }
  if (th0.joinable()) {
    std::cout << "is th0<<<<<<<<<<" << std::endl;
    th0.join();
  }
  return 0;
}
```

### 线程引用参数

```C++
#include <iostream>
#include <thread>   // std::thread
#include <utility>  // std::forward/move
#include <vector>

void func(const std::vector<int>& vec) {
  for (auto& num : vec) {
    std::cout << num << std::endl;
  }
}

int main() {
  std::vector<int> nums = {1, 2, 3, 3, 4};
  std::thread th0(func, std::ref(nums));

  if (th0.joinable()) {
    th0.join();
  }
  return 0;
}
```
### 线程名称等属性设置
