#include "parser.h"
#include <algorithm>
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <csignal>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <iostream>
#include <linux/socket.h>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <sstream>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <utility>
#include <vector>

#define CPPSERVER_CHECK_ERROR(error, name)                                     \
    do {                                                                       \
        if (error < 0) {                                                       \
            std::cerr << "error occured" << std::endl;                         \
            perror(name);                                                      \
            abort();                                                           \
        }                                                                      \
    } while (0)

class ConnSock
{
  public:
    ConnSock() = default;
    ConnSock(int fd, struct sockaddr sockaddr, socklen_t addr_size)

        : fd_(fd), sockaddr_(sockaddr), addr_size_(addr_size)
    {
    }

    ConnSock(const ConnSock&)            = delete;
    ConnSock& operator=(const ConnSock&) = delete;
    ConnSock(ConnSock&& other)
    {
        swap(other);
    }
    ConnSock& operator=(ConnSock&& other)
    {
        close();

        swap(other);
        return *this;
    }
    ~ConnSock()
    {
        close();
    }

    void close()
    {
        if (fd_ != -1) {
            ::close(fd_);
            CPPSERVER_CHECK_ERROR(fd_, "close (socket)");
            fd_ = -1;
        }
        addr_size_ = 0;
        sockaddr_  = {0}; // TODO: check if this does what it's supposed to.
    }

    int getFd() const
    {
        return fd_;
    }

  private:
    void swap(ConnSock& other)
    {
        std::swap(fd_, other.fd_);
        std::swap(addr_size_, other.addr_size_);
        std::swap(sockaddr_, other.sockaddr_);
    }

    int fd_                   = -1;
    int addr_size_            = 0;
    struct sockaddr sockaddr_ = {0};
};

class Socket
{
  public:
    enum SockType {
        kStream,
        kDatagram,
    };

    enum SockOpts {
        kReuseAddr,
        kReusePort,
    };

    Socket() = default;
    Socket(const std::string& addr, int port, SockType type = kStream,
           int queue = 5)
        : port_(port), queue_(queue)
    {
        fd_ = socket(AF_INET, SOCK_STREAM, 0);
        CPPSERVER_CHECK_ERROR(fd_, "socket");

        sockaddr_.sin_family = AF_INET;
        sockaddr_.sin_port   = htons(port_);

        if (addr == "0.0.0.0" || addr == "ALL") {
            sockaddr_.sin_addr.s_addr = INADDR_ANY;
        } else {
            int cerr =
                inet_pton(AF_INET, addr.c_str(), &sockaddr_.sin_addr.s_addr);
            CPPSERVER_CHECK_ERROR(cerr, "inet_pton"); // shouldn't happen
            if (cerr == 0) {
                std::cerr << "not a vaible IPv4 address" << std::endl;
            }
        }

        addr_size_ = sizeof(sockaddr_);
    }

    Socket(const Socket&)            = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other)
    {
        swap(other);
    }
    Socket& operator=(Socket&& other)
    {
        if (fd_ != -1) {
            close(fd_);
            CPPSERVER_CHECK_ERROR(fd_, "close (socket)");
            fd_ = -1;
        }

        port_      = 0;
        addr_size_ = 0;
        sockaddr_  = {0};

        swap(other);
        return *this;
    }
    ~Socket()
    {
        if (fd_ != -1) {
            close(fd_);
            CPPSERVER_CHECK_ERROR(fd_, "close (socket)");
        }
    }

    void bind() const
    {
        int cerr = ::bind(fd_, (struct sockaddr*)&sockaddr_, addr_size_);
        CPPSERVER_CHECK_ERROR(cerr, "bind");
    }

    void listen() const
    {
        int cerr = ::listen(fd_, queue_);
        CPPSERVER_CHECK_ERROR(cerr, "listen");
    }

    // TODO: make a struct of options and point to it. and logic that if you
    // reuse the same option that unsets it.
    template <typename... Opts>
    void setSockOpts(Opts... opts)
    {
        int cerr;
        for (auto& opt : {opts...}) {
            switch (opt) {
            case kReuseAddr: {
                int opt = 1;
                cerr    = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt,
                                     sizeof(opt));
                CPPSERVER_CHECK_ERROR(cerr, "setsockopt");
            }
            case kReusePort: {
                int opt = 1;
                cerr    = setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt,
                                     sizeof(opt));
                CPPSERVER_CHECK_ERROR(cerr, "setsockopt");
            }
            }
        }
    }

    ConnSock accept() const
    {
        struct sockaddr client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket  = ::accept(fd_, &client_addr, &addr_len);
        CPPSERVER_CHECK_ERROR(client_socket, "accept");

        return ConnSock(client_socket, client_addr, addr_len);
    }

    int getFd() const
    {
        return fd_;
    }

  private:
    void swap(Socket& other)
    {
        std::swap(fd_, other.fd_);
        std::swap(port_, other.port_);
        std::swap(addr_size_, other.addr_size_);
        std::swap(sockaddr_, other.sockaddr_);
    }

    int fd_                      = -1;
    int port_                    = 0;
    int addr_size_               = 0;
    int queue_                   = 0;
    struct sockaddr_in sockaddr_ = {0};
};

using std::filesystem::path;

class Server
{
  public:
    Server(const Server&)            = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&)                 = delete;
    Server& operator=(Server&&)      = delete;

    static Server& getInst()
    {
        static Server inst{};
        return inst;
    }

    void get(const path& path,
             std::function<void(const Request&, Response&)>& callback)
    {
    }

    Server& listen(const std::string& addr, int port)
    {
        std::signal(SIGPIPE, pipeHandler);

        listener_ = Socket{addr, port};

        // TODO: Make a dedicated method to set the options.
        listener_.setSockOpts(Socket::kReuseAddr, Socket::kReusePort);

        listener_.bind();
        listener_.listen();

        masterLoop();
        return *this;
    }

  private:
    Server() = default;
    Socket listener_;

    static constexpr int kBuffSize       = 1024 * 4;
    inline static char buffer[kBuffSize] = {0};

    static void pipeHandler(int signal)
    {
        std::cout << "SIGPIPE received" << std::endl;
    }

    void masterLoop()
    {
        const char msg[] = "hello from server!";
        ConnSock conn{};

        // debug -------
        HttpRequestParser parser{};
        Request req{};
        // debug -------

        while (1) {
            conn = listener_.accept();

            while (1) {
                int cerr = recv(conn.getFd(), buffer, sizeof(buffer), 0);
                if (cerr == 0) {
                    std::cout << "client left..." << std::endl;
                    break;
                }
                CPPSERVER_CHECK_ERROR(cerr, "recv");

                std::cout << "Message from client: " << buffer << std::endl;

                // debug -------
                parser.parse(req, buffer, buffer + sizeof(buffer));
                std::cout << req.inspect() << std::endl;
                // debug -------

                cerr = send(conn.getFd(), msg, sizeof(msg), 0);
                if (cerr < 0) {
                    if (errno == EPIPE) {
                        std::cout << "client closed the connection"
                                  << std::endl;
                        break;
                    } else {
                        std::cerr << "error occured" << std::endl;
                        perror("send");
                        abort();
                    }
                }
            }

            std::cout << "closing connection" << std::endl;

            conn.close();
        }
    }
};

int main()
{
    Server& srv = Server::getInst();

    // srv.get("/", [](const Request& req, Response& res) {
    //
    // });

    srv.listen("ALL", 6969);

    return 0;
}
