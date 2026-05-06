#include "parser.h"
#include <algorithm>
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cassert>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <linux/socket.h>
#include <mutex>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <queue>
#include <sstream>
#include <string>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <thread>
#include <unordered_map>
#include <utility>

#define CPPSERVER_CHECK_ERROR(error, name)                                     \
    do {                                                                       \
        if (error < 0) {                                                       \
            std::cerr << "error occured" << std::endl;                         \
            perror(name);                                                      \
            abort();                                                           \
        }                                                                      \
    } while (0)

constexpr char kBoilerplateHtml[] =
    R"(HTTP/1.1 200 OK
Date: Mon, 27 Jul 2024 12:28:53 GMT
Server: Apache/2.4.1
Content-Type: text/html; charset=utf-8
Content-Length: 48

<html><body><h1>Hello World!</h1></body></html>
)";

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

    int get_fd() const
    {
        return fd_;
    }

    void cork() const
    {
        int state = 1;
        int cerr =
            setsockopt(fd_, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));
        CPPSERVER_CHECK_ERROR(cerr, "setsockopt");
    }

    void uncork() const
    {
        int state = 0;
        int cerr =
            setsockopt(fd_, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));
        CPPSERVER_CHECK_ERROR(cerr, "setsockopt");
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

namespace fs = std::filesystem;

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

    using CallbackHandler = std::function<void(const Request&, Response&)>;
    using CallbackMap     = std::unordered_map<std::string, CallbackHandler>;
    // sets a callback function for the server to run when the given path is
    // sent as the uri and the GET method is used.
    void get(const std::string& path, const CallbackHandler& callback)
    {
        GET_callbacks_[path] = callback;
    }

    // returns false if given path is not a directory, otherwise sets the static
    // serve path and returns true.
    // Provide the root_file_name that is to be served for a request of "/". For
    // example index.html.
    bool staticServe(const std::string& path,
                     const std::string& root_file_name = "index.html")
    {
        if (!fs::is_directory(path)) {
            return false;
        }

        fs::path file_name      = fs::path(root_file_name).filename();
        fs::path root_file_path = path / file_name;

        if (!fs::exists(root_file_path)) {
            return false;
        }

        root_file_path_ = root_file_path;

        if (fs::path(path).stem() == "") {
            auto start         = path.begin();
            auto end           = path.end();
            static_serve_path_ = fs::path(start, end);
        } else {
            static_serve_path_ = path;
        }
        return true;
    }

    // starts server.
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
    static constexpr int kMaxBuffSize = 1024 * 8;

    Server()
    {
        rcv_msg_.reserve(kMaxBuffSize);
    }

    static void pipeHandler(int signal)
    {
        std::cout << "SIGPIPE received" << std::endl;
    }

    bool readClientMsg(const ConnSock& conn)
    {
        const char buffer[kMaxBuffSize] = {0};
        bool first_loop                 = true;
        int rcv_flag                    = 0;
        int bytes_rcvd;

        do {
            errno      = 0;
            bytes_rcvd = recv(conn.get_fd(), //
                              (void*)buffer, //
                              sizeof(buffer), rcv_flag);
            std::cout << "BYTES RECIEVED: " << bytes_rcvd << std::endl;
            if (bytes_rcvd == 0) {
                return false;
            } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            }
            CPPSERVER_CHECK_ERROR(bytes_rcvd, "recv");

            rcv_msg_.append(buffer, bytes_rcvd);

            if (first_loop) {
                rcv_flag   = MSG_DONTWAIT;
                first_loop = false;
            }
        } while (bytes_rcvd == kMaxBuffSize);

        return true;
    }

    void masterLoop()
    {
        ConnSock conn{};

        while (1) {
            conn = listener_.accept();

            bool outcome = handleConn(conn);
            // if (outcome == false) {
            //     // do something about the bad client...
            // }

            std::cout << "closing connection" << std::endl;

            conn.close();
        }
    }

    bool compareWiteStaticServePath(const fs::path& p)
    {
        for (auto s_it = static_serve_path_.begin(), p_it = p.begin();
             s_it != static_serve_path_.end(); ++s_it, ++p_it) {
            if (*s_it != *p_it) {
                return false;
            }
        }

        return true;
    }

    using FD = int;
    FD getStaticFileReady(const fs::path& p)
    {
        if (!fs::is_regular_file(p)) {
            return -1;
        }

        FD fd = open(p.c_str(), O_RDONLY);
        CPPSERVER_CHECK_ERROR(fd, "open");

        return fd;
    }

    void createFileResponse(FD fd, size_t file_size, Response& res)
    {
        using ssi = Response::SuccessfulStateIndex;
        using hni = Response::HeaderNameIndex;
        // for now I only support http 1.1
        res.set_versions(1, 1);
        res.set_nameAndTime();

        auto status = std::string(Response::successful_status_names_[ssi::kOK]);
        res.set_status(status);

        res.set_statusCode(200 + ssi::kOK);

        res.set_keepalive(true);

        Response::HeaderItem content_size = {
            .name  = std::string(Response::headers_names_[hni::kContentLength]),
            .value = std::to_string(file_size),
        };
        res.set_headers(content_size);

        res.set_fd(fd);

        res.set_fileSize(file_size);

        res.type = Response::kStatic;
    }

    static void checkSendError(int cerr)
    {
        if (cerr < 0) {
            if (errno == EPIPE) { // errno is thread-safe :D
                std::cout << "client closed the connection" << std::endl;
            } else {
                std::cerr << "error occured" << std::endl;
                perror("send");
                abort();
            }
        }
    }

    bool sendFile(const ConnSock& conn, const Response& res)
    {

        std::string header_str = res.createHeaderString();

        conn.cork();

        int cerr =
            send(conn.get_fd(), header_str.c_str(), header_str.size(), 0);
        checkSendError(cerr);

        cerr = sendfile(conn.get_fd(), res.get_fd(), 0, res.get_fileSize());
        checkSendError(cerr);

        conn.uncork();

        // close sent file
        close(res.get_fd());

        return true;
    }

    bool sendResponse(const ConnSock& conn, const Response& res)
    {
        bool ret_stat = false;

        switch (res.type) {
        case Response::kStatic: {
            return sendFile(conn, res);
            break;
        }
        case Response::kDynamic: {
            // handle dynamic...
            break;
        }
        }

        return ret_stat;
    }

    bool createResponse(const Request& req, Response& res)
    {
        static constexpr char ROOT[] = "/";
        fs::path full_path{};
        fs::path uri = req.get_uri();

        if (uri == ROOT) {
            full_path = root_file_path_;
        } else {
            full_path = static_serve_path_ / uri;
        }

        if (req.get_method() == "GET") {
            if (!static_serve_path_.empty() &&
                compareWiteStaticServePath(full_path)) {
                FD fd = getStaticFileReady(full_path);
                if (fd == -1) {
                    // send 404
                    std::cerr << "404" << std::endl;
                    return false;
                }
                size_t file_size = fs::file_size(full_path);
                // fill up the response object
                createFileResponse(fd, file_size, res);
                return true;
            } else if (GET_callbacks_.find(full_path) != GET_callbacks_.end()) {
                GET_callbacks_[full_path](req, res);
                return true;
            } else {
                // undefined path
                std::cerr << "undefined path" << std::endl;
                return false;
            }
        }

        std::cerr << "undefined method" << std::endl;

        return false;
    }

    // connection handler. Reads input and responds. Returns false if it's a bad
    // http. Returns true if the interaction was OK and the client just closed
    // the connection.
    bool handleConn(const ConnSock& conn)
    {
        bool cres = false;
        Request req{};
        HttpRequestParser parser{};

        while (1) {
            rcv_msg_.clear();

            if (!readClientMsg(conn)) {
                std::cout << "client left..." << std::endl;
                break;
            }

            using ParseRes = HttpRequestParser::ParseResult;
            ParseRes pres  = parser.parse(req, &rcv_msg_[0],
                                          &(rcv_msg_[0]) + rcv_msg_.length());
            // chunked and not done:
            if (pres == ParseRes::ParsingIncompleted) {
                continue;
            } // actual bad http:
            else if (pres == ParseRes::ParsingError) {
                std::cout << "BAD HTTP" << std::endl;
                return false;
            } // Good request:
            else { // send response

                Response res{};
                // debug ------
                std::cout << rcv_msg_ << std::endl;
                // debug ------

                cres = createResponse(req, res);
                if (cres == false) {
                    // handle failiure
                }

                // TODO: check returned result
                sendResponse(conn, res);

                if (req.is_keepalive()) {
                    req.reset();
                    parser.reset();
                } else {
                    return true;
                }
            }
        }

        return false;
    }

    Socket listener_;
    CallbackMap GET_callbacks_;
    std::string rcv_msg_;
    fs::path static_serve_path_;
    fs::path root_file_path_;
};

bool compareWiteStaticServePath(const fs::path& p)
{
    std::string path_str = "/foo/bar/boo/car";
    fs::path path;
    if (fs::path(path_str).stem() == "") {
        auto start = path_str.begin();
        auto end   = --path_str.end();
        path       = fs::path(start, end);
    } else {
        path = path_str;
    }

    std::cout << "path_str: " << path_str << std::endl;
    std::cout << "path: " << path << std::endl;

    for (auto s_it = path.begin(), p_it = p.begin(); s_it != path.end();
         ++s_it, ++p_it) {
        if (*s_it != *p_it) {
            std::cout << "static: " << *s_it << std::endl;
            std::cout << "given: " << *p_it << std::endl;
            return false;
        }
    }

    return true;
}

void test_run(const char* path)
{
    assert(compareWiteStaticServePath(path) == true);
}

int main(int argc, char** argv)
{
    // tests -------
    if (argc > 1 && std::strcmp(argv[1], "test") == 0) {
        test_run(argv[2]);
        return 0;
    }

    Server& srv = Server::getInst();

    // srv.get("/", [](const Request& req, Response& res) {
    //
    // });

    srv.staticServe("../public/");

    srv.listen("ALL", 6969);

    return 0;
}
