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