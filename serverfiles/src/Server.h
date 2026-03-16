#ifndef SERVER_H
#define SERVER_H

#include "Socket.h"
#include <filesystem>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unordered_map>

enum class HttpMethod { GET, POST, PUT };
enum class HttpVersion { LEGACY, ONE, TWO, THREE };
enum class fileType { HTML, CSS, JS };

struct HttpRequest {
    std::string_view method;
    std::string_view path;
    std::string_view version;

    std::unordered_map<std::string_view, std::string_view> headers;
};

class Server {

    bool handle(std::unique_ptr<HttpRequest> req);
    bool handleGet(std::filesystem::path &path);
    // bool handlePost() const;
    // bool handlePut() const;
    void sendError(int num) const;

  public:
    Socket socket;
    std::string buff;

    Server();
    bool parse();
};

#endif
