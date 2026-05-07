#ifndef PARSER_H_
#define PARSER_H_

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <sstream>
#include <string.h>
#include <string>
#include <string_view>
#include <vector>

using std::filesystem::path;

class Response
{
  public:
    struct HeaderItem {
        std::string name;
        std::string value;
    };

    Response()
        : version_major_(0), version_minor_(0), keep_alive_(false),
          statusCode_(0)
    {
    }
    Response(const Response& other)            = default;
    Response& operator=(const Response& other) = default;
    Response(Response&& other)                 = default;
    Response& operator=(Response&& other)      = default;
    ~Response()                                = default;

    std::string createHeaderString() const
    {
        std::stringstream stream;
        stream << "HTTP/" << version_major_ << "." << version_minor_ << " "
               << statusCode_ << " " << status_ << "\r\n ";

        for (auto it = headers_.begin(); it != headers_.end(); ++it) {
            stream << it->name << ": " << it->value << "\r\n";
        }

        stream << "\r\n";

        return stream.str();
    }

    std::string inspect() const
    {
        std::stringstream stream;
        stream << "HTTP/" << version_major_ << "." << version_minor_ << " "
               << statusCode_ << " " << status_ << "\n";

        for (auto it = headers_.begin(); it != headers_.end(); ++it) {
            stream << it->name << ": " << it->value << "\n";
        }

        std::string data(content_.begin(), content_.end());
        stream << data << "\n";
        return stream.str();
    }

    void set_versions(int major, int minor)
    {
        // TODO: maybe add checks on the numbers provided.
        version_major_ = major;
        version_minor_ = minor;
    }

    void set_headers(const HeaderItem& h)
    {
        headers_.push_back(h);
    }

    void set_headers(const std::vector<HeaderItem>& headers)
    {
        for (const auto& h : headers) {
            headers_.push_back(h);
        }
    }

    void set_nameAndTime()
    {
        std::ostringstream oss;
        constexpr std::string_view server_name = "my sick ass server";

        using std::chrono::system_clock;
        const std::string_view* hn = headers_names_;

        const auto cur_time = std::time(nullptr);
        const auto local_time =
            std::put_time(std::localtime(&cur_time), "%d-%m-%Y %H-%M-%S");
        oss << local_time;

        std::vector<HeaderItem> new_headers{
            {.name  = std::string(hn[kServer]),
             .value = std::string(server_name)},
            {.name = std::string(hn[kDate]), .value = oss.str()}};

        set_headers(new_headers);
    }

    void set_content(const std::string& content, const std::string& type)
    {
        const std::string_view* hn = headers_names_;
        set_headers({.name = std::string(hn[kContentType]), .value = type});
        content_ = content;
    }
    void set_keepalive(bool flag)
    {
        keep_alive_ = flag;
    }
    void set_status(const std::string& status)
    {
        status_ = status;
    }

    void set_statusCode(unsigned int status_code)
    {
        statusCode_ = status_code;
    }

    size_t get_fileSize() const
    {
        return file_size_;
    }

    int get_fd() const
    {
        return fd_;
    }

    void set_fileSize(size_t size)
    {
        file_size_ = size;
    }

    void set_fd(int fd)
    {
        fd_ = fd;
    }

    enum HeaderNameIndex {
        kServer,
        kDate,
        kConnection,
        kContentType,
        kContentLength,
        kContentEncoding,
        kTransferEncoding,
        kKeepAlive,
        kLastModified,
        // there are more...
    };

    static constexpr std::string_view headers_names_[] = {
        "Server",
        "Date",
        "Connection",
        "Content-Type",
        "Content-Length",
        "Content-Encoding",
        "Transfer-Encoding",
        "Keep-Alive",
        "Last-Modified",
        // there are more...
    };

    enum SuccessfulStateIndex {
        kOK,
        kCreated,
        kAccepted,
        kNonAuthoritativeInformation,
        kNoContent,
        kResetContent,
        // there are more...
    };

    static constexpr std::string_view successful_status_names_[] = {
        "OK",         "Created",
        "Accepted",   "Non-Authoritative Information",
        "No Content", "Reset Content",
        // there are more...
    };

    enum ClientErrorStateIndex {
        kBadRequiest,
        kUnauthorized,
        kPaymentRequired,
        kForbidden,
        kNotfound,
    };

    static constexpr std::string_view client_error_status_names_[] = {
        "Bad Requiest", "Unauthorized", "Payment Required",
        "Forbidden",    "Not found",
        // there are more...
    };

    enum ResponseType {
        kStatic,
        kDynamic,
    };

    ResponseType type;

  private:
    int version_major_;
    int version_minor_;
    std::vector<HeaderItem> headers_;
    std::string content_;

    unsigned int statusCode_;
    std::string status_;

    bool keep_alive_;

    size_t file_size_;
    int fd_;
};

class Request
{
    friend class HttpRequestParser;

  public:
    struct HeaderItem {
        std::string name;
        std::string value;
    };

    Request() : version_major_(0), version_minor_(0), keep_alive_(false)
    {
    }
    Request(const Request& other)            = default;
    Request& operator=(const Request& other) = default;
    Request(Request&& other)                 = default;
    Request& operator=(Request&& other)      = default;
    ~Request()                               = default;

    std::string inspect() const
    {
        std::stringstream stream;
        stream << method_ << " " << uri_ << " HTTP/" << version_major_ << "."
               << version_minor_ << "\n";

        for (std::vector<Request::HeaderItem>::const_iterator it =
                 headers_.begin();
             it != headers_.end(); ++it) {
            stream << it->name << ": " << it->value << "\n";
        }

        std::string data(content_.begin(), content_.end());
        stream << data << "\n";
        stream << "+ keep-alive: " << keep_alive_ << "\n";
        ;
        return stream.str();
    }

    const std::string& get_method() const
    {
        return method_;
    }
    const std::string& get_uri() const
    {
        return uri_;
    }
    int get_major() const
    {
        return version_major_;
    }
    int get_minor() const
    {
        return version_minor_;
    }
    const std::vector<HeaderItem>& get_headers() const
    {
        return headers_;
    }
    const std::string& get_content() const
    {
        return content_;
    }
    bool is_keepalive() const
    {
        return keep_alive_;
    }
    void reset()
    {
        method_.clear();
        uri_.clear();
        version_major_ = 0;
        version_minor_ = 0;
        headers_.clear();
        content_.clear();
        keep_alive_ = false;
    }

  private:
    std::string method_;
    std::string uri_;
    int version_major_;
    int version_minor_;
    std::vector<HeaderItem> headers_;
    std::string content_;
    bool keep_alive_;
};

class HttpRequestParser
{
  public:
    HttpRequestParser()
        : state(RequestMethodStart), content_size_(0), chunkSize_(0),
          chunked_(false)
    {
    }

    HttpRequestParser(const HttpRequestParser&)            = delete;
    HttpRequestParser& operator=(const HttpRequestParser&) = delete;
    HttpRequestParser(HttpRequestParser&&)                 = default;
    HttpRequestParser& operator=(HttpRequestParser&&)      = default;
    ~HttpRequestParser()                                   = default;

    enum ParseResult { ParsingCompleted, ParsingIncompleted, ParsingError };

    void reset()
    {
        state = RequestMethodStart;
    }

    ParseResult parse(Request& req, const char* begin, const char* end)
    {
        return consume(req, begin, end);
    }

  private:
    static bool checkIfConnection(const Request::HeaderItem& item)
    {
        return strcasecmp(item.name.c_str(), "Connection") == 0;
    }

    ParseResult consume(Request& req, const char* begin, const char* end)
    {
        while (begin != end) {
            char input = *begin++;

            switch (state) {
            case RequestMethodStart:
                if (!isChar(input) || isControl(input) || isSpecial(input)) {
                    return ParsingError;
                } else {
                    state = RequestMethod;
                    req.method_.push_back(input);
                }
                break;
            case RequestMethod:
                if (input == ' ') {
                    state = RequestUriStart;
                } else if (!isChar(input) || isControl(input) ||
                           isSpecial(input)) {
                    return ParsingError;
                } else {
                    req.method_.push_back(input);
                }
                break;
            case RequestUriStart:
                if (isControl(input)) {
                    return ParsingError;
                } else {
                    state = RequestUri;
                    req.uri_.push_back(input);
                }
                break;
            case RequestUri:
                if (input == ' ') {
                    state = RequestHttpVersion_h;
                } else if (input == '\r') {
                    req.version_major_ = 0;
                    req.version_minor_ = 9;

                    return ParsingCompleted;
                } else if (isControl(input)) {
                    return ParsingError;
                } else {
                    req.uri_.push_back(input);
                }
                break;
            case RequestHttpVersion_h:
                if (input == 'H') {
                    state = RequestHttpVersion_ht;
                } else {
                    return ParsingError;
                }
                break;
            case RequestHttpVersion_ht:
                if (input == 'T') {
                    state = RequestHttpVersion_htt;
                } else {
                    return ParsingError;
                }
                break;
            case RequestHttpVersion_htt:
                if (input == 'T') {
                    state = RequestHttpVersion_http;
                } else {
                    return ParsingError;
                }
                break;
            case RequestHttpVersion_http:
                if (input == 'P') {
                    state = RequestHttpVersion_slash;
                } else {
                    return ParsingError;
                }
                break;
            case RequestHttpVersion_slash:
                if (input == '/') {
                    req.version_major_ = 0;
                    req.version_minor_ = 0;
                    state              = RequestHttpVersion_majorStart;
                } else {
                    return ParsingError;
                }
                break;
            case RequestHttpVersion_majorStart:
                if (isDigit(input)) {
                    req.version_major_ = input - '0';
                    state              = RequestHttpVersion_major;
                } else {
                    return ParsingError;
                }
                break;
            case RequestHttpVersion_major:
                if (input == '.') {
                    state = RequestHttpVersion_minorStart;
                } else if (isDigit(input)) {
                    req.version_major_ = req.version_major_ * 10 + input - '0';
                } else {
                    return ParsingError;
                }
                break;
            case RequestHttpVersion_minorStart:
                if (isDigit(input)) {
                    req.version_minor_ = input - '0';
                    state              = RequestHttpVersion_minor;
                } else {
                    return ParsingError;
                }
                break;
            case RequestHttpVersion_minor:
                if (input == '\r') {
                    state = ResponseHttpVersion_newLine;
                } else if (isDigit(input)) {
                    req.version_minor_ = req.version_minor_ * 10 + input - '0';
                } else {
                    return ParsingError;
                }
                break;
            case ResponseHttpVersion_newLine:
                if (input == '\n') {
                    state = HeaderLineStart;
                } else {
                    return ParsingError;
                }
                break;
            case HeaderLineStart:
                if (input == '\r') {
                    state = ExpectingNewline_3;
                } else if (!req.headers_.empty() &&
                           (input == ' ' || input == '\t')) {
                    state = HeaderLws;
                } else if (!isChar(input) || isControl(input) ||
                           isSpecial(input)) {
                    return ParsingError;
                } else {
                    req.headers_.push_back(Request::HeaderItem());
                    req.headers_.back().name.reserve(16);
                    req.headers_.back().value.reserve(16);
                    req.headers_.back().name.push_back(input);
                    state = HeaderName;
                }
                break;
            case HeaderLws:
                if (input == '\r') {
                    state = ExpectingNewline_2;
                } else if (input == ' ' || input == '\t') {
                } else if (isControl(input)) {
                    return ParsingError;
                } else {
                    state = HeaderValue;
                    req.headers_.back().value.push_back(input);
                }
                break;
            case HeaderName:
                if (input == ':') {
                    state = SpaceBeforeHeaderValue;
                } else if (!isChar(input) || isControl(input) ||
                           isSpecial(input)) {
                    return ParsingError;
                } else {
                    req.headers_.back().name.push_back(input);
                }
                break;
            case SpaceBeforeHeaderValue:
                if (input == ' ') {
                    state = HeaderValue;
                } else {
                    return ParsingError;
                }
                break;
            case HeaderValue:
                if (input == '\r') {
                    if (req.method_ == "POST" || req.method_ == "PUT") {
                        Request::HeaderItem& h = req.headers_.back();

                        if (strcasecmp(h.name.c_str(), "Content-Length") == 0) {
                            content_size_ = atoi(h.value.c_str());
                            req.content_.reserve(content_size_);
                        } else if (strcasecmp(h.name.c_str(),
                                              "Transfer-Encoding") == 0) {
                            if (strcasecmp(h.value.c_str(), "chunked") == 0)
                                chunked_ = true;
                        }
                    }
                    state = ExpectingNewline_2;
                } else if (isControl(input)) {
                    return ParsingError;
                } else {
                    req.headers_.back().value.push_back(input);
                }
                break;
            case ExpectingNewline_2:
                if (input == '\n') {
                    state = HeaderLineStart;
                } else {
                    return ParsingError;
                }
                break;
            case ExpectingNewline_3: {
                std::vector<Request::HeaderItem>::iterator it =
                    std::find_if(req.headers_.begin(), req.headers_.end(),
                                 checkIfConnection);

                if (it != req.headers_.end()) {
                    if (strcasecmp(it->value.c_str(), "Keep-Alive") == 0) {
                        req.keep_alive_ = true;
                    } else { // == Close
                        req.keep_alive_ = false;
                    }
                } else {
                    if (req.version_major_ > 1 ||
                        (req.version_major_ == 1 && req.version_minor_ == 1))
                        req.keep_alive_ = true;
                }

                if (chunked_) {
                    state = ChunkSize;
                } else if (content_size_ == 0) {
                    if (input == '\n')
                        return ParsingCompleted;
                    else
                        return ParsingError;
                } else {
                    state = Post;
                }
                break;
            }
            case Post:
                --content_size_;
                req.content_.push_back(input);

                if (content_size_ == 0) {
                    return ParsingCompleted;
                }
                break;
            case ChunkSize:
                if (isalnum(input)) {
                    chunk_size_str_.push_back(input);
                } else if (input == ';') {
                    state = ChunkExtensionName;
                } else if (input == '\r') {
                    state = ChunkSizeNewLine;
                } else {
                    return ParsingError;
                }
                break;
            case ChunkExtensionName:
                if (isalnum(input) || input == ' ') {
                    // skip
                } else if (input == '=') {
                    state = ChunkExtensionValue;
                } else if (input == '\r') {
                    state = ChunkSizeNewLine;
                } else {
                    return ParsingError;
                }
                break;
            case ChunkExtensionValue:
                if (isalnum(input) || input == ' ') {
                    // skip
                } else if (input == '\r') {
                    state = ChunkSizeNewLine;
                } else {
                    return ParsingError;
                }
                break;
            case ChunkSizeNewLine:
                if (input == '\n') {
                    chunkSize_ = strtol(chunk_size_str_.c_str(), NULL, 16);
                    chunk_size_str_.clear();
                    req.content_.reserve(req.content_.size() + chunkSize_);

                    if (chunkSize_ == 0)
                        state = ChunkSizeNewLine_2;
                    else
                        state = ChunkData;
                } else {
                    return ParsingError;
                }
                break;
            case ChunkSizeNewLine_2:
                if (input == '\r') {
                    state = ChunkSizeNewLine_3;
                } else if (isalpha(input)) {
                    state = ChunkTrailerName;
                } else {
                    return ParsingError;
                }
                break;
            case ChunkSizeNewLine_3:
                if (input == '\n') {
                    return ParsingCompleted;
                } else {
                    return ParsingError;
                }
                break;
            case ChunkTrailerName:
                if (isalnum(input)) {
                    // skip
                } else if (input == ':') {
                    state = ChunkTrailerValue;
                } else {
                    return ParsingError;
                }
                break;
            case ChunkTrailerValue:
                if (isalnum(input) || input == ' ') {
                    // skip
                } else if (input == '\r') {
                    state = ChunkSizeNewLine;
                } else {
                    return ParsingError;
                }
                break;
            case ChunkData:
                req.content_.push_back(input);

                if (--chunkSize_ == 0) {
                    state = ChunkDataNewLine_1;
                }
                break;
            case ChunkDataNewLine_1:
                if (input == '\r') {
                    state = ChunkDataNewLine_2;
                } else {
                    return ParsingError;
                }
                break;
            case ChunkDataNewLine_2:
                if (input == '\n') {
                    state = ChunkSize;
                } else {
                    return ParsingError;
                }
                break;
            default:
                return ParsingError;
            }
        }

        return ParsingIncompleted;
    }

    // Check if a byte is an HTTP character.
    inline bool isChar(int c)
    {
        return c >= 0 && c <= 127;
    }

    // Check if a byte is an HTTP control character.
    inline bool isControl(int c)
    {
        return (c >= 0 && c <= 31) || (c == 127);
    }

    // Check if a byte is defined as an HTTP special character.
    inline bool isSpecial(int c)
    {
        switch (c) {
        case '(':
        case ')':
        case '<':
        case '>':
        case '@':
        case ',':
        case ';':
        case ':':
        case '\\':
        case '"':
        case '/':
        case '[':
        case ']':
        case '?':
        case '=':
        case '{':
        case '}':
        case ' ':
        case '\t':
            return true;
        default:
            return false;
        }
    }

    // Check if a byte is a digit.
    inline bool isDigit(int c)
    {
        return c >= '0' && c <= '9';
    }

    // The current state of the parser.
    enum State {
        RequestMethodStart,
        RequestMethod,
        RequestUriStart,
        RequestUri,
        RequestHttpVersion_h,
        RequestHttpVersion_ht,
        RequestHttpVersion_htt,
        RequestHttpVersion_http,
        RequestHttpVersion_slash,
        RequestHttpVersion_majorStart,
        RequestHttpVersion_major,
        RequestHttpVersion_minorStart,
        RequestHttpVersion_minor,

        ResponseStatusStart,
        ResponseHttpVersion_ht,
        ResponseHttpVersion_htt,
        ResponseHttpVersion_http,
        ResponseHttpVersion_slash,
        ResponseHttpVersion_majorStart,
        ResponseHttpVersion_major,
        ResponseHttpVersion_minorStart,
        ResponseHttpVersion_minor,
        ResponseHttpVersion_spaceAfterVersion,
        ResponseHttpVersion_statusCodeStart,
        ResponseHttpVersion_spaceAfterStatusCode,
        ResponseHttpVersion_statusTextStart,
        ResponseHttpVersion_newLine,

        HeaderLineStart,
        HeaderLws,
        HeaderName,
        SpaceBeforeHeaderValue,
        HeaderValue,
        ExpectingNewline_2,
        ExpectingNewline_3,

        Post,
        ChunkSize,
        ChunkExtensionName,
        ChunkExtensionValue,
        ChunkSizeNewLine,
        ChunkSizeNewLine_2,
        ChunkSizeNewLine_3,
        ChunkTrailerName,
        ChunkTrailerValue,

        ChunkDataNewLine_1,
        ChunkDataNewLine_2,
        ChunkData,
    } state;

    size_t content_size_;
    std::string chunk_size_str_;
    size_t chunkSize_;
    bool chunked_;
};

#endif
