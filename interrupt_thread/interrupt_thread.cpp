#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>

class InterruptFlag {
 public:
  void set() {
    b_.store(true, std::memory_order_relaxed);
    std::lock_guard<std::mutex> l(m_);
    if (cv_) {
      cv_->notify_all();
    }
  }
  bool is_set() const { return b_.load(std::memory_order_relaxed); };

  void set_condition_variable(std::condition_variable& cv) {
    std::lock_guard<std::mutex> l(m_);
    cv_ = &cv;
  }
  void clear_condition_variable() {
    std::lock_guard<std::mutex> l(m_);
    cv_ = nullptr;
  }

 private:
  std::atomic<bool> b_;
  std::condition_variable* cv_ = nullptr;
  std::mutex m_;
};

thread_local InterruptFlag this_thread_interrupt_flag;

struct ClearConditionVariableOnDestruct {
  ~ClearConditionVariableOnDestruct() { this_thread_interrupt_flag.clear_condition_variable(); }
};

void interruption_point() {
  if (this_thread_interrupt_flag.is_set()) {
    throw std::runtime_error("thread has beed interrupt!");
  }
}

void interruptible_wait(std::condition_variable& cv, std::mutex& mutex) {
  interruption_point();
  this_thread_interrupt_flag.set_condition_variable(cv);
  // 之后的 wait_for 可能抛异常，所以需要 RAII 清除标志
  ClearConditionVariableOnDestruct guard;
  interruption_point();
  // 设置线程看到中断前的等待时间上限
  std::unique_lock<std::mutex> l(mutex);
  cv.wait_for(l, std::chrono::milliseconds(1));
  interruption_point();
}

template <typename Predicate>
void interruptible_wait(std::condition_variable& cv, std::mutex& mutex, Predicate pred) {
  interruption_point();
  this_thread_interrupt_flag.set_condition_variable(cv);
  ClearConditionVariableOnDestruct guard;
  std::unique_lock<std::mutex> l(mutex);
  while (!this_thread_interrupt_flag.is_set() && !pred()) {
    cv.wait_for(l, std::chrono::milliseconds(1));
  }
  interruption_point();
}

class InterruptibleThread {
 public:
  template <typename F>
  InterruptibleThread(F f) {
    std::promise<InterruptFlag*> p;
    t = new std::thread([f, &p] {
      p.set_value(&this_thread_interrupt_flag);
      try {
        f();
      } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
      }
    });
    flag = p.get_future().get();
  }
  ~InterruptibleThread() {
    if (t->joinable()) {
      t->join();
    }
  }

  void interrupt() {
    if (flag) {
      flag->set();
    }
  }

 private:
  std::thread* t;
  InterruptFlag* flag;
};

std::mutex config_mutex;
std::vector<InterruptibleThread> background_threads;

void background_thread() {
  std::condition_variable cv;
  interruptible_wait(cv, config_mutex, []() -> bool { return false; });
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void start_background_processing() {
  background_threads.push_back(InterruptibleThread(background_thread));
  background_threads.push_back(InterruptibleThread(background_thread));
}

int main() {
  start_background_processing();
  std::unique_lock<std::mutex> l(config_mutex);
  for (auto& x : background_threads) {
    x.interrupt();
  }
}