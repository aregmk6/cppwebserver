#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "mysocket.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace amk {

class ThreadPool {
public:
  ThreadPool(int pool_size);
  ~ThreadPool();

  void add_client(ClientSocket &client_conn);
  void wait_for_workers() {
    for (auto &t : m_workers) {
      t.join();
    }
  }
  bool is_busy();
  bool m_should_terminate = false;

private:
  void threadLoop(int thread_num);

  int m_pool_size;
  std::vector<std::thread> m_workers;
  std::queue<ClientSocket> m_client_queue;
  std::mutex m_queue_mutex;
  std::condition_variable m_queue_cond;
  const std::function<bool()> m_loop_predicate;
};

} // namespace amk

#endif
