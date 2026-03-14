#ifndef SOCKET_H
#define SOCKET_H

#include "consts.h"
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

struct Address {
    struct sockaddr_in addr = {0};
    socklen_t addrLen = 0;
};

class Socket {
    int sockfd;
    int clientfd = -1;
    Address clientAddr = {0};
    Address serverAddr = {0};

  public:
    Socket(); /* TCP socket */
    ~Socket();

    void Bind() const;
    void Listen() const;
    void Accept();

    int readHeader(std::string &buf) const;
    bool Send(std::string &buf) const;
};

#endif
