#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool
{
public:
    ThreadPool(size_t thread_count)
    {
        start(thread_count);
    }

    ThreadPool()
    {}

    ~ThreadPool()
    {
        stop();
    }

    //We need to be able to process lambdas
    void dispatch_async(std::function<void()> func)
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.emplace(std::move(func));
        }
        condition.notify_one();
    }

    void start(size_t numThreads)
    {
        for(size_t i = 0; i < numThreads; ++i)
        {
            workers.emplace_back([this]
            {
                while(true)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queueMutex);
                        this->condition.wait(lock, [this]
                        {
                            return this->stopFlag || !this->tasks.empty();
                        });

                        if(this->stopFlag && this->tasks.empty())
                        {
                            return;
                        }

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            });
        }
    }

    void stop() noexcept
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stopFlag = true;
        }
        condition.notify_all();

        for(auto& thread : workers)
        {
            thread.join();
        }
    }

private:
    std::vector<std::thread>          workers;
    std::queue<std::function<void()>> tasks;

    std::mutex              queueMutex;
    std::condition_variable condition;
    std::atomic<bool>       stopFlag = false;
};
