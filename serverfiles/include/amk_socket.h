#ifndef SOCKET_H
#define SOCKET_H

#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

namespace amk
{

struct address {
  struct sockaddr_in addr = {0};
  socklen_t addrLen       = 0;
};

class Socket
{
public:
  int sockfd;
  int clientfd = -1;

  Socket(); /* TCP socket */

  void Bind() const;
  void Listen() const;
  void Accept();
  void Close();

  void cork() const;
  void uncork() const;

  int readHeader(std::string &buf) const;
  bool Send(const std::string &buf) const;
  bool SendFile(int fd, int size) const;

private:
  address clientAddr = {0};
  address serverAddr = {0};
};

} // namespace amk

#endif
