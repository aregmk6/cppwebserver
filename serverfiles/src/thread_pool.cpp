#include "thread_pool.h"
#include "mysocket.h"
#include "parser.h"
#include "utils.h"
#include <string>
#include <thread>
#include <utility>

using namespace amk;

ThreadPool::ThreadPool(int pool_size)
    : m_pool_size(pool_size), m_workers(m_pool_size),
      m_loop_predicate(
          [this]() { return !m_client_queue.empty() && !m_should_terminate; }) {
  for (int i = 0; i < m_pool_size; ++i) {
    m_workers.emplace_back(std::thread(&ThreadPool::threadLoop, this, i));
  }
}

ThreadPool::~ThreadPool() {
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

void ThreadPool::add_client(ClientSocket &client_conn) {
  {
    std::lock_guard lock(m_queue_mutex);
    m_client_queue.push(std::move(client_conn));
  }

  m_queue_cond.notify_one();
}

bool ThreadPool::is_busy() {
  std::lock_guard queue_lock(m_queue_mutex);
  return !m_client_queue.empty();
}

void ThreadPool::threadLoop(int thread_num) {
  Logger t_logger{std::to_string(thread_num).c_str()};

  std::string cur_req_buff;
  ClientSocket cur_client;
  ReqHandler handler;
  while (true) {
    {
      std::unique_lock lock(m_queue_mutex);
      m_queue_cond.wait(lock, m_loop_predicate);

      if (m_should_terminate) {
        t_logger.log("stopping now...");
        return;
      }

      cur_client = std::move(m_client_queue.back());
      m_client_queue.pop();

      t_logger.log(std::string("getting client number: ") +
                   std::to_string(cur_client.m_client_fd));
    }

    /* according to mozilla, the only two options for Connection header are
     * keep-alive or close. If we get a keep-alive request, we should requeue
     * the connection */

    t_logger.log("handling client:");

    bool keep_alive = handler.handle_conn(cur_client);
    if (keep_alive) {
      {
        t_logger.log("keep-alive -> putting back");
        std::lock_guard lock(m_queue_mutex);
        m_client_queue.push(std::move(cur_client));
        m_queue_cond.notify_one();
      }
    } else {
      t_logger.log("closing connection");
      cur_client.Close();
    }
  }
}
