#ifndef SERVER_H
#define SERVER_H

#include "mysocket.h"
#include "thread_pool.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>

namespace amk {

class Server {
public:
  static Server &
  create_instance(int port_,
                  int thread_pool_size = std::thread::hardware_concurrency()) {
    static Server inst{port_, thread_pool_size};
    inst_p = &inst;

    return inst;
  }

  static Server &get_instance() { return *inst_p; }

  ClientSocket wait_for_connection();
  void send_client(ClientSocket &client_conn);
  bool should_stop() const { return stop_flag; }
  void shutdown() { m_thpool.wait_for_workers(); }

  Server() = delete;
  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;

private:
  Server(int port_, int thread_pool_size = std::thread::hardware_concurrency());

  static void int_handler(int signal) {
    Server &inst = get_instance();
    inst.stop_server();

    std::cout << "SIGINT recieved, shutting down..." << std::endl;
  }

  void stop_server() {
    m_thpool.m_should_terminate = true;
    stop_flag = true;
  }
  ThreadPool m_thpool;
  ListeningSocket m_listening_socket;
  int port;
  bool stop_flag = false;

  inline static Server *inst_p = nullptr;
};

} // namespace amk

#endif
