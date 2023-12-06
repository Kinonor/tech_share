#include <condition_variable>
#include <functional>
#include <mutex>
#include <atomic>
#include <future>
#include <memory>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

class ThreadPool {
 public:
  ThreadPool() {
    std::size_t n = std::thread::hardware_concurrency();
    try {
      for (std::size_t i = 0; i < n; ++i) {
        threads_.emplace_back(&ThreadPool::worker_thread, this);
      }
    } catch (...) {
      done_ = true;
      for (auto& x : threads_) {
        if (x.joinable()) {
          x.join();
        }
      }
      throw;
    }
  }

  ~ThreadPool() {
    done_ = true;
    cv_.notify_all();
    for (auto& x : threads_) {
      if (x.joinable()) {
        x.join();
      }
    }
  }

  template <class F, class... Args>
  auto submit(F&& f, Args&&... args) {
    using RT = std::invoke_result_t<F, Args...>;
    auto task = std::make_shared<std::packaged_task<RT()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    if (local_queue_ && local_queue_->size() < 1000) {
      local_queue_->emplace([task]() { (*task)(); });
    } else {
      std::lock_guard<std::mutex> l(m_);
      pool_queue_.emplace([task]() { (*task)(); });
    }
    cv_.notify_one();
    return task->get_future();
  }

 private:
  void worker_thread() {
    local_queue_.reset(new std::queue<std::function<void()>>);
    while (!done_) {
      std::function<void()> task;
      if (local_queue_ && !local_queue_->empty()) {
        task = std::move(local_queue_->front());
        local_queue_->pop();
        task();
        continue;
      }
      std::unique_lock<std::mutex> l(m_);
      if (pool_queue_.size()) {
        task = std::move(pool_queue_.front());
        pool_queue_.pop();
        task();
      } else {
        cv_.wait(l);
      }
    }
  }

 private:
  std::mutex m_;
  std::condition_variable cv_;
  std::atomic<bool> done_ = false;
  std::queue<std::function<void()>> pool_queue_;
  inline static thread_local std::unique_ptr<std::queue<std::function<void()>>> local_queue_;
  std::vector<std::thread> threads_;
};

int main()
{
    ThreadPool pool;

    for (int i = 0; i < 100000; ++i) {
        pool.submit([](int y)->int{
        ++y;
        return y;
    }, 1);
    }
    return 0;
}