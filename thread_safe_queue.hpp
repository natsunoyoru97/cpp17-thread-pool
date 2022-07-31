#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

// 线程安全的队列，可用于线程池
// 注意：由于用到了 std::optional，需要 C++17 及以上版本
template <typename T>
class Tqueue {
  public:
    // 构造函数
    Tqueue() = default;

    // 析构函数
    ~Tqueue() = default;

    // 移动函数
    Tqueue(Tqueue&& q) = default;

    void push(const T& elem) {
        std::lock_guard<std::mutex> lock(mutex_);
        q_.push(std::move(elem));
        cond_.notify_all();
    }

    T WaitAndPop() {
        std::unique_lock<std::mutex> lock(mutex_);
        //std::cerr << "Waiting... \n";
        cond_.wait(lock, [this]() {
            return !(this->q_).empty();
        });
        //std::cerr << "Finish waiting\n";
        auto value = std::move(q_.front());
        q_.pop();

        return value;
    }

    std::optional<T> TryPop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (q_.empty()) {
            return { };
        };
        auto value = std::move(q_.front());
        q_.pop();

        return value;
    }

    std::optional<T> front() {
        std::unique_lock<std::mutex> lock(mutex_);
        // 考虑队列为空的情况
        if (q_.empty()) {
            return { };
        }
        
        return q_.front();
    }

    std::optional<T> back() {
        std::unique_lock<std::mutex> lock(mutex_);
        // 考虑队列为空的情况
        if (q_.empty()) {
            return { };
        }

        return q_.back();
    }

    bool empty() {
        std::unique_lock<std::mutex> lock(mutex_);
        return q_.empty();
    }

    size_t size() {
        std::unique_lock<std::mutex> lock(mutex_);
        return q_.size();
    }

  private:
    std::queue<T> q_;
    std::mutex mutex_;
    std::condition_variable cond_;

    // 禁用复制函数，mutex对象不允许复制
    Tqueue(const Tqueue& q);
    // 禁用赋值
    Tqueue& operator=(const Tqueue&);
};

// 这里的行为都不应该允许
void TestForbiddenBehavior() {
    Tqueue<int> tq1;
    //Tqueue<int> tq2 = tq1;
    //Tqueue<int> tq3(tq1);
}