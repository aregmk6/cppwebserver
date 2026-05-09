#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <queue>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define CPPSERVER_CHECK_ERROR(error, name)                                     \
    do {                                                                       \
        if (error < 0) {                                                       \
            std::cerr << "error occured" << std::endl;                         \
            perror(name);                                                      \
            abort();                                                           \
        }                                                                      \
    } while (0)

class ConnSock
{
  public:
    ConnSock() = default;
    ConnSock(int fd, struct sockaddr sockaddr, socklen_t addr_size)

        : fd_(fd), addr_size_(addr_size), sockaddr_(sockaddr)
    {
    }

    ConnSock(const ConnSock&)            = delete;
    ConnSock& operator=(const ConnSock&) = delete;
    ConnSock(ConnSock&& other)
    {
        swap(other);
    }
    ConnSock& operator=(ConnSock&& other)
    {
        close();

        swap(other);
        return *this;
    }
    ~ConnSock()
    {
        close();
    }

    void close()
    {
        if (fd_ != -1) {
            ::close(fd_);
            CPPSERVER_CHECK_ERROR(fd_, "close (socket)");
            fd_ = -1;
        }
        addr_size_ = 0;
        sockaddr_  = {0}; // TODO: check if this does what it's supposed to.
    }

    int get_fd() const
    {
        return fd_;
    }

    void cork() const
    {
        int state = 1;
        int cerr =
            setsockopt(fd_, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));
        CPPSERVER_CHECK_ERROR(cerr, "setsockopt");
    }

    void uncork() const
    {
        int state = 0;
        int cerr =
            setsockopt(fd_, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));
        CPPSERVER_CHECK_ERROR(cerr, "setsockopt");
    }

  private:
    void swap(ConnSock& other)
    {
        std::swap(fd_, other.fd_);
        std::swap(addr_size_, other.addr_size_);
        std::swap(sockaddr_, other.sockaddr_);
    }

    int fd_                   = -1;
    int addr_size_            = 0;
    struct sockaddr sockaddr_ = {0};
};

template <typename T>
class TsQueue
{
  public:
    TsQueue()                             = default;
    TsQueue(const TsQueue<T>&)            = delete;
    TsQueue& operator=(const TsQueue<T>&) = delete;
    TsQueue(TsQueue<T>&&)                 = default;
    TsQueue& operator=(TsQueue<T>&&)      = default;

    void push(T&& new_item)
    {
        std::lock_guard<std::mutex> lk(m_);
        queue_.push(std::forward<T>(new_item));
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

class ThreadJoiner
{
  private:
    std::vector<std::thread>& threads_;

  public:
    explicit ThreadJoiner(std::vector<std::thread>& threads) : threads_(threads)
    {
    }

    ThreadJoiner(const ThreadJoiner&)            = delete;
    ThreadJoiner& operator=(const ThreadJoiner&) = delete;

    ~ThreadJoiner()
    {
        for (auto& th : threads_) {
            if (th.joinable()) {
                th.join();
            }
        }
    }
};

class ThreadPool
{
  public:
    void submit(ConnSock&& conn)
    {
        connections_.push(std::move(conn));
    }

    ThreadPool() : done_(false), joiner_(threads_)
    {
        const int th_num = std::thread::hardware_concurrency();
        for (int i = 0; i < th_num; ++i) {
            threads_.push_back(std::thread(&ThreadPool::worker_thread, this));
        }
    }

    ~ThreadPool()
    {
        done_ = true;
    }

  private:
    std::atomic_bool done_;
    TsQueue<ConnSock> connections_;
    std::vector<std::thread> threads_;
    ThreadJoiner joiner_;

    void worker_thread()
    {
    }
};

#endif
