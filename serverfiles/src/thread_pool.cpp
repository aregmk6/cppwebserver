#include "thread_pool.h"

#include "amk_socket.h"

using namespace amk;

ThreadPool::ThreadPool(int pool_size)
    : m_pool_size(pool_size), m_workers(m_pool_size), loop_predicate([this]() {
        return !m_client_queue.empty() && !m_should_terminate;
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

void amk::ThreadPool::add_client(ClientSocket &client_conn)
{
  {
    std::lock_guard lock(m_queue_mutex);
    m_client_queue.push(client_conn);
  }

  m_queue_cond.notify_one();
}

bool amk::ThreadPool::is_busy() const
{
  std::lock_guard queue_lock(m_queue_mutex);
  return !m_client_queue.empty();
}

void amk::ThreadPool::threadLoop()
{
  std::string cur_req_buff;
  ClientSocket new_conn;
  while (true) {
    {
      std::unique_lock lock(m_queue_mutex);
      m_queue_cond.wait(lock, loop_predicate);
      if (m_should_terminate) {
        // thread just finishes
        return;
      }
      new_conn = m_client_queue.back();
      m_client_queue.pop();
    }

    m_handler.handle(new_conn);

    new_conn.Close();
  }
}
