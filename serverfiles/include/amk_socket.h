#ifndef SOCKET_H
#define SOCKET_H

#include "file.h"
#include "response.h"

#include <asm-generic/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace amk
{

struct Address {
  struct sockaddr_in addr = {0};
  socklen_t addrLen       = 0;
};

class ClientSocket
{
public:
  ClientSocket() = default;
  ~ClientSocket();
  ClientSocket(int client_fd, Address client_addr);
  int m_client_fd = -1;

  void Close();

  int read_header();
  void send_response(const File &src);
  const std::string &get_buf() const;

private:
  std::string buf;
  void cork() const;
  void uncork() const;
  bool send_header(const Response &res) const;
  bool send_body(const File &src) const;
  void set_keep_alive() const;
  void unset_keep_alive() const;

  Address clientAddr = {0};
};

class ListeningSocket
{
public:
  ListeningSocket(); /* TCP socket */
  ~ListeningSocket();
  int m_listen_fd;

  void Bind() const;
  void Listen() const;
  ClientSocket Accept();
  void Close();

private:
  Address serverAddr = {0};
};

} // namespace amk

#endif
