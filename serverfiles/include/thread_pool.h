#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "amk_socket.h"
#include "request_handler.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace amk
{

class ThreadPool
{
public:
  ThreadPool(int pool_size);
  ~ThreadPool();

  void add_client(ClientSocket &client_conn);
  bool is_busy() const;

private:
  void threadLoop();

  int m_pool_size;
  bool m_should_terminate = false;
  std::vector<std::thread> m_workers;
  std::queue<ClientSocket> m_client_queue;
  std::mutex m_queue_mutex;
  std::condition_variable m_queue_cond;
  const std::function<bool()> loop_predicate;
  ReqHandler m_handler;
};

} // namespace amk

#endif
