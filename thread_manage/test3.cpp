#include <pthread.h>

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
  // std::thread th0(func, std::ref(nums));
  std::thread th0([&nums]() {
    nums.push_back(5);
    std::cout << nums[5] << std::endl;
    std::cout << nums.size() << std::endl;
    while (1) {
      sleep(1);
    };
  });
  pthread_setname_np(th0.native_handle(), "hello");

  if (th0.joinable()) {
    th0.join();
  }
  std::cout << nums.size() << std::endl;
  return 0;
}