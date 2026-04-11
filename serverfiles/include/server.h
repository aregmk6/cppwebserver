#ifndef SERVER_H
#define SERVER_H

#include "amk_socket.h"
#include "thread_pool.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>

namespace amk
{

class Server
{
public:
  Server(int thread_pool_size = std::thread::hardware_concurrency());
  [[nodiscard]] ClientSocket wait_for_connection();
  void send_client(ClientSocket &client_conn);

private:
  ThreadPool m_thpool;
  ListeningSocket m_listening_socket;
};

} // namespace amk

#endif
