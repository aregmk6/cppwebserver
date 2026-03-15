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
    Address clientAddr = {0};
    Address serverAddr = {0};

  public:
    int sockfd;
    int clientfd = -1;

    Socket(); /* TCP socket */

    void Bind() const;
    void Listen() const;
    void Accept();
    void Close();

    int readHeader(std::string &buf) const;
    bool Send(std::string &buf) const;
};

#endif
