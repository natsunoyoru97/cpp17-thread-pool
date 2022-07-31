#include "thread_safe_queue.hpp"
#include <future>
#include <functional>

class ThreadPool {
    public:
        ThreadPool(size_t threads): stopped_(false) {
            for (int i = 0; i < threads; ++i) {
                workers_.emplace_back(
                    // 任务内容
                    [this] {
                        for ( ;; ) {
                            std::function<void()> task;

                            {
                                std::unique_lock<std::mutex> lock(mutex_);
                                this->cond_.wait(
                                    lock, [this]() {
                                        return this->stopped_ || !tasks_.empty();
                                    }
                                );
                                if (this->stopped_ && tasks_.empty()) {
                                    return;
                                }
                                task = tasks_.WaitAndPop();
                            }

                            task();
                        }
                    }
                );
            }
            //std::cerr << threads << " threads are created\n";
        }

        ~ThreadPool() {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                stopped_ = true;
            }
            cond_.notify_all();
            //std::cerr << "The thread pool has been stopped\n";

            for (std::thread& worker : workers_) {
                worker.join();
                //std::cerr << "Collecting the worker\n";
            }
            //std::cerr << "All workers are collected\n";
        }

        // 禁用拷贝
        ThreadPool(const ThreadPool&) = delete;

        // 禁用赋值
        ThreadPool& operator=(const ThreadPool&) = delete;

        template<class F, class... Args> 
        auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type> {
            // 推断返回类型
            using ReturnType = typename std::result_of<F(Args...)>::type;

            auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
                
            std::future<ReturnType> res = task->get_future();
            {
                std::unique_lock<std::mutex> lock(mutex_);

                // 线程池停止，不应该继续加进任务
                if (stopped_) {
                    return { };
                }

                tasks_.push([task](){ (*task)(); });
                cond_.notify_one();
            }

            return res;
        }

    private:
        Tqueue<std::function<void()>> tasks_;
        std::vector<std::thread> workers_;
        std::mutex mutex_;
        std::condition_variable cond_;
        bool stopped_;
};

void TestBasicUse() {
    ThreadPool pool(4);
    auto result = pool.enqueue([](int answer) { return answer; }, 42);

    std::cout << result.get() << std::endl;
}

void TestMultipleUse() {
    ThreadPool pool(4);
    std::vector<std::future<int>> results;

    for(int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue([i] {
                std::cout << "hello " << i << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "world " << i << std::endl;
                return i * i;
            })
        );
    }

    for (auto && result: results) {
        std::cout << result.get() << ' ';
    }
        
    std::cout << std::endl;
}

int main() {
    TestBasicUse();
    TestMultipleUse();
}