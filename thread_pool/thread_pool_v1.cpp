#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

class ThreadPool {
 public:
  explicit ThreadPool(std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      std::thread{[this] {
        std::unique_lock<std::mutex> l(m_);
        while (true) {
          if (!q_.empty()) {
            auto task = std::move(q_.front());
            q_.pop();
            l.unlock();
            task();
            l.lock();
          } else if (done_ && q_.empty()) {
            break;
          } else {
            cv_.wait(l);
          }
        }
      }}.detach();
    }
  }

  ~ThreadPool() {
    {
      std::lock_guard<std::mutex> l(m_);
      done_ = true;  // cv_.wait 使用了 done_ 判断所以要加锁
    }
    cv_.notify_all();
  }

  template <class F, class... Args>
  auto submit(F&& f, Args&&... args) {
    using RT = std::invoke_result_t<F, Args...>;
    auto task = std::make_shared<std::packaged_task<RT()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    {
      std::lock_guard<std::mutex> l(m_);
      q_.emplace([task]() { (*task)(); });
    }
    cv_.notify_one();
    return task->get_future();
  }

 private:
  std::mutex m_;
  std::condition_variable cv_;
  bool done_ = false;
  std::queue<std::function<void()>> q_;
};

int main() {
  ThreadPool pool(10);

  for (int i = 0; i < 100000; ++i) {
    pool.submit(
        [](int y, int z, int f) -> int {
          ++y;
          return y;
        },
        1, 3, 4);
  }
  return 0;
}