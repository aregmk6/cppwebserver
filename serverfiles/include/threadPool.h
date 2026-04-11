#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace amk
{

class threadPool
{
public:
  threadPool(int pool_size = std::thread::hardware_concurrency());
  ~threadPool();

  template <typename F, typename... Args>
  void add_job(F &&f, Args &&...args);
  bool is_busy() const;

private:
  void threadLoop();

  int m_pool_size;
  bool m_should_terminate = false;
  std::vector<std::thread> m_workers;
  std::queue<std::function<void()>> m_jobs_queue;
  std::mutex m_queue_mutex;
  std::condition_variable m_queue_cond;
  const std::function<bool()> loop_predicate;
};

template <typename F, typename... Args>
inline void threadPool::add_job(F &&f, Args &&...args)
{
  /* I want to be able to do jobs that are not only void(), so I wrap it in a
   * lambda.
   */
  auto job = [f = std::forward<F>(f), ... args = std::forward<Args>(args)]() {
    std::invoke(std::move(f), std::move(args)...);
  };

  // TODO: find a way to wrap it and allow for return values in the future.

  {
    std::lock_guard lock(m_queue_mutex);
    m_jobs_queue.push(job);
  }

  m_queue_cond.notify_one();
}

} // namespace amk

#endif
