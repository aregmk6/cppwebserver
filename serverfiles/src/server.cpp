#include "server.h"

#include "mysocket.h"

#include <csignal>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>

using namespace amk;

Server::Server(int port_, int thread_pool_size)
    : m_listening_socket(port_), port(port_), m_thpool(thread_pool_size) {
  m_listening_socket.Bind();
  m_listening_socket.Listen();

  std::signal(SIGINT, int_handler);
}

ClientSocket amk::Server::wait_for_connection() {
  return m_listening_socket.Accept();
}

void amk::Server::send_client(ClientSocket &client_conn) {
  m_thpool.add_client(client_conn);
}
