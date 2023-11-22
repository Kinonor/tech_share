#include <future>
#include <iostream>

int main() {
  std::packaged_task<int(int)> task([](int i) { return i; });
  task(1);  // 请求计算结果，内部的 future 将设置结果值
  std::future<int> res = task.get_future();
  std::cout << res.get() << std::endl;
}