#ifndef SERVER_H
#define SERVER_H

#include "Socket.h"
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

class Server {
    Socket socket;

    std::string buff;

    enum method { GET, POST, PUT };

    void handleGet() const;
    void handlePost() const;
    void sendError(int num) const;

  public:
    Server();
    bool handle();
};

#endif
