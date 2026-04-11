#include "threadPool.h"

using namespace amk;

ThreadPool::ThreadPool(int pool_size)
    : m_pool_size(pool_size), m_workers(m_pool_size), loop_predicate([this]() {
        return !m_jobs_queue.empty() && !m_should_terminate;
      })
{
  for (int i = 0; i < m_pool_size; ++i) {
    m_workers.emplace_back(std::thread(&ThreadPool::threadLoop, this));
  }
}
amk::ThreadPool::~ThreadPool()
{
  {
    const std::lock_guard lock(m_queue_mutex);
    m_should_terminate = true;
  }

  m_queue_cond.notify_all();
  for (auto &thread : m_workers) {
    thread.join();
  }
  m_workers.clear();
}
bool amk::ThreadPool::is_busy() const
{
  std::lock_guard queue_lock(m_queue_mutex);
  return !m_jobs_queue.empty();
}

void amk::ThreadPool::threadLoop()
{
  while (true) {
    std::function<void()> job;
    {
      std::unique_lock lock(m_queue_mutex);
      m_queue_cond.wait(lock, loop_predicate);
      if (m_should_terminate) {
        // thread just finishes
        return;
      }
      job = m_jobs_queue.back();
      m_jobs_queue.pop();
    }
    job();
  }
}
