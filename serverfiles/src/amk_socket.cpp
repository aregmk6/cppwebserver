#include "amk_socket.h"
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

Socket::Socket()
{
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  serverAddr.addr.sin_family      = AF_INET;
  serverAddr.addr.sin_port        = htons(8080);
  serverAddr.addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  serverAddr.addrLen              = sizeof(serverAddr.addr);
}

void Socket::Bind() const
{
  int r = bind(sockfd, (struct sockaddr *)&serverAddr.addr, serverAddr.addrLen);

  if (r < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }
}

void Socket::Accept()
{
  clientfd =
      accept(sockfd, (struct sockaddr *)&clientAddr.addr, &clientAddr.addrLen);

  if (clientfd < 0) {
    perror("accept");
    exit(EXIT_FAILURE);
  }
}

void Socket::Listen() const
{
  int r = listen(sockfd, 5);

  if (r < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
}

void Socket::Close()
{
  close(clientfd);
  clientfd = -1;
}

int Socket::readHeader(std::string &buf) const
{
  buf.resize(MAX_HEADER_SIZE);
  int totalBytes = 0;
  while (1) {
    int br = recv(clientfd, &buf[0], MAX_HEADER_SIZE, 0);
    if (br < 0) {
      perror("recv");
      exit(EXIT_FAILURE);
    }

    totalBytes += br;

    if (buf.find(HEADER_END) != std::string::npos) {
      break;
    }

    if (totalBytes == MAX_HEADER_SIZE) {
      return -1;
    }
  }
  std::cout << buf << std::endl;

  return totalBytes;
}

bool Socket::Send(const std::string &buf) const
{
  int startFrom = 0;
  int bytesLeft = buf.size();

  while (1) {
    int bs = send(clientfd, buf.c_str() + startFrom, bytesLeft, 0);

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

bool Socket::SendFile(int fd, int size) const
{
  int startFrom = 0;
  int bytesLeft = size;

  while (1) {
    int bs = sendfile(clientfd, fd, 0, bytesLeft);

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

void Socket::cork() const
{
  int state = 1;
  setsockopt(clientfd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));
  std::cout << "corked" << std::endl;
}

void Socket::uncork() const
{
  int state = 0;
  setsockopt(clientfd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));
  std::cout << "uncorked" << std::endl;
}
