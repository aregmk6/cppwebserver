#include "amk_socket.h"

#include "response.h"
#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace amk;

ListeningSocket::ListeningSocket()
{
  m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (m_listen_fd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  serverAddr.addr.sin_family      = AF_INET;
  serverAddr.addr.sin_port        = htons(8080);
  serverAddr.addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  serverAddr.addrLen              = sizeof(serverAddr.addr);
}

ListeningSocket::~ListeningSocket()
{
  close(m_listen_fd);
}

void ListeningSocket::Bind() const
{
  int r = bind(
      m_listen_fd, (struct sockaddr *)&serverAddr.addr, serverAddr.addrLen);

  if (r < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }
}

ClientSocket ListeningSocket::Accept()
{
  Address clientAddr = {0};
  int client_fd      = accept(
      m_listen_fd, (struct sockaddr *)&clientAddr.addr, &clientAddr.addrLen);

  if (client_fd < 0) {
    perror("accept");
    exit(EXIT_FAILURE);
  }

  return ClientSocket(client_fd, clientAddr);
}

void ListeningSocket::Listen() const
{
  int r = listen(m_listen_fd, 5);

  if (r < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
}

void ListeningSocket::Close()
{
  close(m_listen_fd);
  m_listen_fd = -1;
}

void amk::ClientSocket::Close()
{
  close(m_client_fd);
  m_client_fd = -1;
}

int ClientSocket::read_header()
{
  buf.resize(max_header_size);
  int totalBytes = 0;
  while (true) {
    int br = recv(m_client_fd, &buf[0], max_header_size, 0);
    if (br < 0) {
      perror("recv");
      exit(EXIT_FAILURE);
    }

    totalBytes += br;

    if (buf.find(header_end) != std::string::npos) {
      break;
    }

    if (totalBytes == max_header_size) {
      return -1;
    }
  }
  std::cout << buf << std::endl;

  return totalBytes;
}
void amk::ClientSocket::send_response(const File &src)
{
  Response resp(src);

  cork();
  send_header(resp);
  send_body(src);
  uncork();
}

bool ClientSocket::send_header(const Response &resp) const
{
  int startFrom = 0;
  int bytesLeft = resp.header().size();

  while (true) {
    int bs = send(m_client_fd, resp.header().c_str() + startFrom, bytesLeft, 0);

    if (bs <= 0) {
      return false;
    }

    if (bytesLeft <= 0) {
      break;
    }

    startFrom += bs;
    bytesLeft -= bs;
  }

  std::cout << "header sent" << std::endl;
  return true;
}

bool ClientSocket::send_body(const File &src) const
{
  int startFrom = 0;
  int bytesLeft = src.get_size();

  while (1) {
    int bs = sendfile(m_client_fd, src.fd(), 0, bytesLeft);

    if (bs <= 0) {
      return false;
    }

    if (bytesLeft <= 0) {
      break;
    }

    bytesLeft -= bs;
  }

  std::cout << "file sent" << std::endl;
  return true;
}

void ClientSocket::cork() const
{
  int state = 1;
  setsockopt(m_client_fd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));
  std::cout << "corked" << std::endl;
}

void ClientSocket::uncork() const
{
  int state = 0;
  setsockopt(m_client_fd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));
  std::cout << "uncorked" << std::endl;
}

void amk::ClientSocket::set_keep_alive() const
{
  int state = 1;
  setsockopt(m_client_fd, SOL_SOCKET, SO_KEEPALIVE, &state, sizeof(state));
  std::cout << "keep alive set" << std::endl;
}

void amk::ClientSocket::unset_keep_alive() const
{
  int state = 0;
  setsockopt(m_client_fd, SOL_SOCKET, SO_KEEPALIVE, &state, sizeof(state));
  std::cout << "keep alive unset" << std::endl;
}

amk::ClientSocket::~ClientSocket()
{
  close(m_client_fd);
}

ClientSocket::ClientSocket(int client_fd, Address client_addr)
    : m_client_fd(client_fd), clientAddr(client_addr)
{
}
