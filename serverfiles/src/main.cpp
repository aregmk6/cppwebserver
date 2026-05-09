#include "parser.h"
#include "thread_pool.h"
#include <algorithm>
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cassert>
#include <condition_variable>
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
#include <string>
#include <syncstream>
#include <sys/sendfile.h>
#include <sys/socket.h>
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

class ConnSock
{
  public:
    ConnSock() = default;
    ConnSock(int fd, struct sockaddr sockaddr, socklen_t addr_size)

        : fd_(fd), addr_size_(addr_size), sockaddr_(sockaddr)
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

        ConnSock conn = ConnSock(client_socket, client_addr, addr_len);
        // conn.set_timeout();

        return conn;
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

    int fd_        = -1;
    int port_      = 0;
    int addr_size_ = 0;
    int queue_     = 0;

    struct sockaddr_in sockaddr_ = {0};
};

template <typename T>
class TsQueue
{
  public:
    TsQueue()                             = default;
    TsQueue(const TsQueue<T>&)            = delete;
    TsQueue& operator=(const TsQueue<T>&) = delete;
    TsQueue(TsQueue<T>&&)                 = default;
    TsQueue& operator=(TsQueue<T>&&)      = default;

    void push(T&& new_item)
    {
        std::lock_guard<std::mutex> lk(m_);
        queue_.push(std::forward<T>(new_item));
        c_.notify_one();
    }

    // waits until an item is available.
    void waitAndPop(T& item)
    {
        std::unique_lock<std::mutex> lk(m_);
        c_.wait(lk, [this]() { return !queue_.empty(); });
        item = std::move(queue_.front());
        queue_.pop();
    }

    std::shared_ptr<T> waitAndPop()
    {
        T item;
        waitAndPop(item);
        return std::make_shared<T>(item);
    }

    // try to put value into item immediatly, and return true if it worked.
    // otherwise false.
    bool tryPop(T& item)
    {
        std::lock_guard<std::mutex> lk(m_);
        if (!queue_.empty()) {
            item = std::move(queue_.front());
            queue_.pop();
            return true;
        }
        return false;
    }

    std::shared_ptr<T> tryPop()
    {
        T item;
        if (tryPop(item)) {
            return std::make_shared<T>(item);
        }
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(m_);
        return queue_.empty();
    }

  private:
    // mutable mutex for const empty method
    mutable std::mutex m_;
    std::condition_variable c_;
    std::queue<T> queue_;
};

class ThreadJoiner
{
  private:
    std::vector<std::thread>& threads_;

  public:
    explicit ThreadJoiner(std::vector<std::thread>& threads) : threads_(threads)
    {
    }

    ThreadJoiner(const ThreadJoiner&)            = delete;
    ThreadJoiner& operator=(const ThreadJoiner&) = delete;

    ~ThreadJoiner()
    {
        for (auto& th : threads_) {
            if (th.joinable()) {
                th.join();
            }
        }
    }
};

namespace fs = std::filesystem;

class ThreadPool
{
    // TODO: provide setters for the static paths instead
    friend class Server;

  public:
    void submit(ConnSock&& conn)
    {
        connections_.push(std::move(conn));
    }

    ThreadPool() : done_(false), joiner_(threads_)
    {
        try {
            const int th_num = std::thread::hardware_concurrency();
            for (int i = 0; i < th_num; ++i) {
                threads_.push_back(
                    std::thread(&ThreadPool::worker_thread, this));
            }
        } catch (...) {
            done_ = true;
            throw;
        }
    }

    ThreadPool(const ThreadPool& other)            = delete;
    ThreadPool& operator=(const ThreadPool& other) = delete;

    ~ThreadPool()
    {
        done_ = true;
    }

  private:
    static constexpr int kMaxBuffSize = 1024 * 8;

    bool handleConn(const ConnSock& conn)
    {
        HttpRequestParser parser;

        int cres = false;
        Request req{};

        std::string rcvd_msg;
        rcvd_msg.reserve(kMaxBuffSize);

        while (1) {
            rcvd_msg.clear();

            if (!readClientMsg(conn, rcvd_msg)) {
                std::cout << "client left..." << std::endl;
                break;
            }

            using ParseRes = HttpRequestParser::ParseResult;
            ParseRes pres  = parser.parse(req, &rcvd_msg[0],
                                          &(rcvd_msg[0]) + rcvd_msg.length());
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
                std::osyncstream(std::cout) << rcvd_msg << std::endl;
                // debug ------

                cres = createResponse(req, res);
                if (cres != 0) {
                    switch (cres) {
                    case 404:
                        // send 404
                        std::cout << "404" << std::endl;
                        return false;
                        break;
                    default:
                        break;
                    }
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

    bool readClientMsg(const ConnSock& conn, std::string& rcvd_msg)
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
            if (bytes_rcvd == 0) {
                return false;
            } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            } else if (errno == ECONNRESET) {
                std::cerr << "connection reset by peer... handling."
                          << std::endl;
                break;
            }
            CPPSERVER_CHECK_ERROR(bytes_rcvd, "recv");

            rcvd_msg.append(buffer, bytes_rcvd);

            if (first_loop) {
                rcv_flag   = MSG_DONTWAIT;
                first_loop = false;
            }
        } while (bytes_rcvd == kMaxBuffSize);

        return true;
    }

    int createResponse(const Request& req, Response& res)
    {
        static constexpr char ROOT[] = "/";
        fs::path full_path{};
        fs::path uri = req.get_uri();

        if (uri == ROOT) {
            full_path = root_file_path_;
        } else {
            full_path =
                static_serve_path_ / std::filesystem::relative(uri, "/");
        }

        if (req.get_method() == "GET") {
            if (!static_serve_path_.empty() &&
                compareWiteStaticServePath(full_path)) {
                FD fd = getStaticFileReady(full_path);
                if (fd == -1) {
                    // send 404
                    std::cerr << "non existent file" << std::endl;
                    return 404;
                }
                size_t file_size = fs::file_size(full_path);
                // fill up the response object
                createFileResponse(fd, file_size, full_path, res);
                return 0;
            } else if (GET_callbacks_.find(full_path) != GET_callbacks_.end()) {
                GET_callbacks_[full_path](req, res);
                return 0;
            } else {
                // undefined path
                std::cerr << "undefined path" << std::endl;
                return 404;
            }
        }

        std::cerr << "undefined method" << std::endl;

        return false;
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

    void worker_thread()
    {
        ConnSock conn{};

        while (1) {
            connections_.waitAndPop(conn);

            std::osyncstream(std::cout)
                << "Hi I'm thread number " << std::this_thread::get_id()
                << " grabbed a connection with fd: " << conn.get_fd()
                << std::endl;

            // TODO: requeue when it's keep alive
            handleConn(conn);

            // bool outcome = handleConn(conn);
            // if (outcome == false) {
            //     // do something about the bad client...
            // }

            std::osyncstream(std::cout)
                << "Hi I'm thread number " << std::this_thread::get_id()
                << " and I'm closing the connection" << std::endl;

            conn.close();
        }
    }

    bool sendFile(const ConnSock& conn, const Response& res)
    {

        std::string header_str = res.createHeaderString();

        conn.cork();

        uint cerr =
            send(conn.get_fd(), header_str.c_str(), header_str.size(), 0);
        checkSendError(cerr);

        cerr = sendfile(conn.get_fd(), res.get_fd(), 0, res.get_fileSize());
        checkSendError(cerr);

        conn.uncork();

        // close sent file
        close(res.get_fd());

        assert(cerr == res.get_fileSize());

        return true;
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

    void createFileResponse(FD fd, size_t file_size, const fs::path& path,
                            Response& res)
    {
        auto extp = ExtensionParser{};

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

        Response::HeaderItem content_type = {
            .name  = std::string(Response::headers_names_[hni::kContentType]),
            .value = extp.parseExtension(path.extension()),
        };

        res.set_headers(content_type);

        res.set_fd(fd);

        res.set_fileSize(file_size);

        res.type = Response::kStatic;
    }

    using CallbackHandler = std::function<void(const Request&, Response&)>;
    using CallbackMap     = std::unordered_map<std::string, CallbackHandler>;

    CallbackMap GET_callbacks_;

    std::atomic_bool done_;
    TsQueue<ConnSock> connections_;
    std::vector<std::thread> threads_;
    ThreadJoiner joiner_;

    inline static fs::path static_serve_path_ = fs::path{};
    inline static fs::path root_file_path_    = fs::path{};
};

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
        pool_.GET_callbacks_[path] = callback;
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

        pool_.root_file_path_ = root_file_path;

        if (fs::path(path).stem() == "") {
            auto start               = path.begin();
            auto end                 = --path.end();
            pool_.static_serve_path_ = fs::path(start, end);
        } else {
            pool_.static_serve_path_ = path;
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
    Server() = default;

    static void pipeHandler(int signal)
    {
        std::cout << "SIGPIPE received" << std::endl;
    }

    // TODO: make it add a connection to the queue
    void masterLoop()
    {
        ConnSock conn{};

        while (1) {
            conn = listener_.accept();

            std::cout << "new connection!\nopened fd: " << conn.get_fd()
                      << std::endl;

            pool_.submit(std::move(conn));
        }
    }

    Socket listener_;
    CallbackMap GET_callbacks_;
    ThreadPool pool_;
};

void test_run(const char* path)
{
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

    assert(srv.staticServe(
               "/home/aregmk/coding/web/cppserver/serverfiles/public/") ==
           true);

    srv.listen("ALL", 6969);

    return 0;
}
