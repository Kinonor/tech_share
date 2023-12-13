#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

#include "function_wrapper.hpp"

class WorkStealingQueue {
 public:
  WorkStealingQueue() = default;

  WorkStealingQueue(const WorkStealingQueue&) = delete;

  WorkStealingQueue& operator=(const WorkStealingQueue&) = delete;

  void push(FunctionWrapper f) {
    std::lock_guard<std::mutex> l(m_);
    q_.push_front(std::move(f));
  }

  bool empty() const {
    std::lock_guard<std::mutex> l(m_);
    return q_.empty();
  }

  bool try_pop(FunctionWrapper& res) {
    std::lock_guard<std::mutex> l(m_);
    if (q_.empty()) {
      return false;
    }
    res = std::move(q_.front());
    q_.pop_front();
    return true;
  }

  bool try_steal(FunctionWrapper& res) {
    std::lock_guard<std::mutex> l(m_);
    if (q_.empty()) {
      return false;
    }
    res = std::move(q_.back());
    q_.pop_back();
    return true;
  }

 private:
  std::deque<FunctionWrapper> q_;
  mutable std::mutex m_;
};

class ThreadPool {
 public:
  ThreadPool() {
    std::size_t n = std::thread::hardware_concurrency();
    try {
      for (std::size_t i = 0; i < n; ++i) {
        work_stealing_queue_.emplace_back(std::make_unique<WorkStealingQueue>());
        threads_.emplace_back(&ThreadPool::worker_thread, this, i);
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

  template <typename F, class... Args>
  std::future<std::invoke_result_t<F, Args...>> submit(F&& f, Args&&... args) {
    using RT = std::invoke_result_t<F, Args...>;
    std::packaged_task<RT()> task(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<std::invoke_result_t<F, Args...>> res(task.get_future());
    if (local_queue_) {
      local_queue_->push(std::move(task));
    } else {
      std::lock_guard<std::mutex> l(m_);
      pool_queue_.push(std::move(task));
    }
    cv_.notify_one();
    return res;
  }

 private:
  bool pop_task_from_local_queue(FunctionWrapper& task) { return local_queue_ && local_queue_->try_pop(task); }

  bool pop_task_from_pool_queue(FunctionWrapper& task) {
    if (pool_queue_.empty()) return false;

    task = std::move(pool_queue_.front());
    pool_queue_.pop();
    return true;
  }

  bool pop_task_from_other_thread_queue(FunctionWrapper& task) {
    for (std::size_t i = 0; i < work_stealing_queue_.size(); ++i) {
      std::size_t index = (index_ + i + 1) % work_stealing_queue_.size();
      if (work_stealing_queue_[index]->try_steal(task)) {
        return true;
      }
    }
    return false;
  }

  void worker_thread(std::size_t index) {
    index_ = index;
    local_queue_ = work_stealing_queue_[index_].get();

    while (!done_) {
      FunctionWrapper task;
      if (pop_task_from_local_queue(task) || pop_task_from_other_thread_queue(task)) {
        task();
        continue;
      }

      std::unique_lock<std::mutex> l(m_);
      if (pop_task_from_pool_queue(task)) {
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
  std::queue<FunctionWrapper> pool_queue_;
  std::vector<std::unique_ptr<WorkStealingQueue>> work_stealing_queue_;
  std::vector<std::thread> threads_;

  static thread_local WorkStealingQueue* local_queue_;
  static thread_local std::size_t index_;
};

thread_local WorkStealingQueue* ThreadPool::local_queue_;
thread_local std::size_t ThreadPool::index_;

int main() {
  ThreadPool pool;

  std::vector<std::future<int>> res;

  for (std::size_t i = 0; i < 10; ++i) {
    std::future<int> fu = pool.submit(std::bind([](int y) -> int { return y; }, i));
    res.push_back(std::move(fu));
  }

  for (auto& result : res) {
    result.wait();
    std::cout << result.get() << std::endl;
  }

  return 0;
}