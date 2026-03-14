#ifndef SERVER_H
#define SERVER_H

#include "Socket.h"
#include <memory>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unordered_map>

enum class HttpMethod { GET, POST, PUT };

struct HttpRequest {
    std::string_view method;
    std::string_view path;
    std::string_view version;

    std::unordered_map<std::string_view, std::string_view> headers;
};

class Server {
    Socket socket;
    std::string buff;

    bool handle(std::unique_ptr<HttpRequest> req) const;
    void handleGet() const;
    void handlePost() const;
    void sendError(int num) const;

  public:
    Server();
    bool parse();
};

#endif
