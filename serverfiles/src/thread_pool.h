#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ConnSock;

template <typename T>
class TsQueue
{
  public:
    TsQueue<T>()                             = default;
    TsQueue<T>(const TsQueue<T>&)            = delete;
    TsQueue<T>& operator=(const TsQueue<T>&) = delete;
    TsQueue<T>(TsQueue<T>&&)                 = default;
    TsQueue<T>& operator=(TsQueue<T>&&)      = default;

    void push(T new_item)
    {
        std::lock_guard<std::mutex> lk(m_);
        queue_.push(new_item);
        c_.notify_one();
    }

    // waits until an item is available.
    void waitAndPop(T& item)
    {
        std::lock_guard<std::mutex> lk(m_);
        c_.wait(m_, [this]() { !queue_.empty(); });
        item = queue_.front();
        queue_.pop();
    }

    std::shared_ptr<T> waitAndPop()
    {
        T item;
        waitAndPop(item);
        return std::make_shared<T>(item);
    }

    // try to put value into item immediatly, and return true if it worked.
    // otherwise false.
    bool tryPop(T& item)
    {
        std::lock_guard<std::mutex> lk(m_);
        if (!queue_.empty()) {
            item = queue_.front();
            queue_.pop();
            return true;
        }
        return false;
    }

    std::shared_ptr<T> tryPop()
    {
        T item;
        if (tryPop(item)) {
            return std::make_shared<T>(item);
        }
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(m_);
        return queue_.empty();
    }

  private:
    // mutable mutex for const empty method
    mutable std::mutex m_;
    std::condition_variable c_;
    std::queue<T> queue_;
};

class ThreadPool
{
  public:
  private:
    std::atomic_bool done_;
    TsQueue<ConnSock> connections_;
    std::vector<std::thread> threads_;
};

#endif
