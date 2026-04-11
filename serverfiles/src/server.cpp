#include "server.h"

#include "amk_socket.h"

#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>

using namespace amk;

Server::Server(int thread_pool_size) : m_thpool(thread_pool_size)
{
  m_listening_socket.Bind();
  m_listening_socket.Listen();
}

ClientSocket amk::Server::wait_for_connection()
{
  return m_listening_socket.Accept();
}

void amk::Server::send_client(ClientSocket &client_conn)
{
  m_thpool.add_client(client_conn);
}
