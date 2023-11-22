# 线程间的数据共享与同步

## 数据共享
### 锁
#### std::scope_lock(C++17)
在需要对多个mutex进行加锁时，使用 std::scope_lock 可以不用担心加锁顺序不同导致死锁。、
- 先对其中一个锁，调用lock，然后对另一个调用 try_lock
- try_lock 为false时，前面一个锁也解锁

```C++
#include <iostream>
#include <mutex>

class A {
 public:
  void lock() { std::cout << 1; }
  void unlock() { std::cout << 2; }
  bool try_lock() {
    std::cout << 3;
    return true;
  }
};

class B {
 public:
  void lock() { std::cout << 4; }
  void unlock() { std::cout << 5; }
  bool try_lock() {
    std::cout << 6;
    return true;
  }
};

int main() {
  A a;
  B b;
  {
    std::scoped_lock l(a, b);  // 16
    std::cout << std::endl;
  }  // 25
}
```
####  std::shared_mutex(C++17)

```c++
#include <iostream>
#include <shared_mutex>

class A {
 public:
  void lock_shared() { std::cout << "lock_shared" << std::endl; }
  void unlock_shared() { std::cout << "unlock_shared" << std::endl; }
};

int main() {
  A a;
  {
    std::shared_lock l(a);  // lock_shared
  }                         // unlock_shared
}
```

```C++
class A {
 public:
  int read() const {
    std::shared_lock<std::shared_mutex> l(m_);
    return n_;
  }

  int write() {
    std::unique_lock<std::shared_mutex> l(m_);
    return ++n_;
  }

 private:
  mutable std::shared_mutex m_;
  int n_ = 0;
};
```

#### std::recursive_mutex
- 同一线程递归加锁

```C++
#include <mutex>

class A {
 public:
  void f() {
    m_.lock();
    m_.unlock();
  }

  void g() {
    m_.lock();
    f();
    m_.unlock();
  }

 private:
  std::recursive_mutex m_;
};

int main() {
  A{}.g();
}
```

## 数据初始化

### 双重检查的危害
```C++  
#include <memory>
#include <mutex>
#include <thread>

class A {
 public:
  void f() {}
};

std::shared_ptr<A> p;
std::mutex m;

void init() {
  if (!p) {  // 未上锁，其他线程可能在执行 #1，则此时 p 不为空
    std::lock_guard<std::mutex> l(m);
    if (!p) {
      p.reset(new A);  // 1
      // 先分配内存，再在内存上构造 A 的实例并返回内存的指针，最后让 p 指向它
      // 也可能先让 p 指向它，再在内存上构造 A 的实例
    }
  }
  p->f();  // p 可能指向一块还未构造实例的内存，从而崩溃
}

int main() {
  std::thread t1{init};
  std::thread t2{init};

  t1.join();
  t2.join();
}
```
### 安全的单例
```c++
template <typename T>
class Singleton {
 public:
  static T& Instance();
  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;

 private:
  Singleton() = default;
  ~Singleton() = default;
};

template <typename T>
T& Singleton<T>::Instance() {
  static T instance; // c++11 开始支持线程安全，解决了双检查的问题
  return instance;
}
```

## 线程同步
除了常用的条件变量、信号量（C++20）等同步之外，C++20 还支持了 std::barrier
### std::barrier
```C++
#include <barrier>
#include <cassert>
#include <iostream>
#include <thread>

class A {
 public:
  void f() {
    std::barrier sync_point{3, [&]() noexcept { ++i_; }};
    for (auto& x : tasks_) {
      x = std::thread([&] {
        std::cout << 1;
        sync_point.arrive_and_wait(); // 所有的线程都到达次同步点后，再往下执行
        assert(i_ == 1);
        std::cout << 2;
        sync_point.arrive_and_wait();
        assert(i_ == 2);
        std::cout << 3;
      });
    }
    for (auto& x : tasks_) {
      x.join();
    }
  }

 private:
  std::thread tasks_[3] = {};
  int i_ = 0;
};

int main() {
  A a;
  a.f();
}
```

### std::latch

```C++
#include <iostream>
#include <latch>
#include <string>
#include <thread>

class A {
 public:
  void f() {
    for (auto& x : data_) {
      x.t = std::jthread([&] {
        x.s += x.s;
        done_.count_down();
      });
    }
    done_.wait(); // 阻塞至计数为0，保证所有的线程执行完
    for (auto& x : data_) {
      std::cout << x.s << std::endl;
    }
  }

 private:
  struct {
    std::string s;
    std::jthread t;
  } data_[3] = {
      {"hello"},
      {"down"},
      {"demo"},
  };

  std::latch done_{3};
};

int main() {
  A a;
  a.f();
}
```

### std::promise
```C++
#include <chrono>
#include <future>
#include <iostream>

class A {
 public:
  void task() { std::cout << 1; }
  void wait_for_task() {
    ps_.get_future().wait();
    task();
  }
  void signal() { ps_.set_value(); }

 private:
  std::promise<void> ps_; // 只是通知作用，返回值为 void
};

void task() { std::cout << 1; }

int main() {
  A a;
  std::thread t(&A::wait_for_task, &a);
  a.signal();
  t.join();
}
```

### std::packaged_task
```C++
#include <future>
#include <iostream>

int main() {
  std::packaged_task<int(int)> task([](int i) { return i; });
  task(1);  // 请求计算结果，内部的 future 将设置结果值
  std::future<int> res = task.get_future();
  std::cout << res.get();  // 1
}
```