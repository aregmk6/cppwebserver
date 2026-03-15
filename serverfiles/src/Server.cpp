#include "Server.h"
#include "consts.h"
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

static const std::unordered_map<std::string_view, HttpMethod> methodMap = {
    {"GET", HttpMethod::GET},
    {"POST", HttpMethod::POST},
    {"PUT", HttpMethod::PUT}};

static const std::unordered_map<std::string_view, HttpVersion> versionMap = {
    {"HTTP/1.0", HttpVersion::LEGACY},
    {"HTTP/1.1", HttpVersion::ONE},
    {"HTTP/2", HttpVersion::TWO},
    {"HTTP/3", HttpVersion::THREE}};

Server::Server() : socket(), buff() {
    buff.resize(MAX_HEADER_SIZE + 1, 0);
}

void Server::sendError(int num) const {
    return;
}

bool Server::parse() {
    int headerSize = socket.readHeader(buff);
    if (headerSize < 0) {
        std::cerr << "Read Failed" << std::endl;
    }

    std::string_view window(buff.c_str());
    std::string_view firstLine;
    std::string_view curLine;
    std::string_view curWord;
    size_t lineEnd = std::string::npos;
    size_t wordEnd = std::string::npos;
    auto req = std::make_unique<HttpRequest>();

    /* handle first line */
    lineEnd = window.find(DELIM);
    if (lineEnd == std::string::npos) {
        sendError(EHEADER);
        return false;
    }

    firstLine = window.substr(0, lineEnd);
    window.remove_prefix(lineEnd + 2);
    int count = 0;
    while (count < FL_HEADER_NR) {
        if (count != FL_HEADER_NR - 1) {
            wordEnd = firstLine.find(SPACE);

            if (wordEnd == std::string::npos) {
                sendError(EHEADER);
                return false;
            }

            curWord = firstLine.substr(0, wordEnd);
        } else {
            curWord = firstLine;
        }

        switch (count) {
        case 0:
            req->method = curWord;
            break;
        case 1:
            req->path = curWord;
            break;
        case 2:
            req->version = curWord;
            break;
        }

        firstLine.remove_prefix(wordEnd + 1); // size of a single space = 1
        count++;
    }

    /*
        GET /index.html HTTP/1.1\r\n
        Host: aregm.com\r\n
        User-Agent: Mozilla/5.0 (Arch Linux)\r\n
        Accept: text/html,application/xhtml+xml\r\n
        Accept-Language: en-US,en;q=0.9\r\n
        Connection: keep-alive\r\n
        \r\n
        [Optional Body Data Starts Here]
    */

    /* handle rest of the lines */
    size_t colon = 0;
    std::string_view key, value;
    while (1) {
        lineEnd = window.find(DELIM);

        if (lineEnd == std::string::npos) {
            sendError(EHEADER);
            return false;
        }

        if (!lineEnd) break;

        curLine = window.substr(0, lineEnd);

        colon = curLine.find(':');
        if (colon == std::string::npos) {
            sendError(EHEADER);
            return false;
        }

        key = curLine.substr(0, colon);
        value = curLine.substr(colon + 1, lineEnd - colon - 1);

        req->headers.insert({key, value});

        window.remove_prefix(lineEnd + 2); // size of "\r\n" = 2
    }

    // TODO: add body parser

    return handle(std::move(req));
}

using namespace std::filesystem;

bool Server::handle(std::unique_ptr<HttpRequest> req) const {
    auto it_method = methodMap.find(req->method);
    if (it_method == methodMap.end()) {
        sendError(EHEADER); // TODO: switch to other error
        return false;
    }

    auto it_version = versionMap.find(req->version);
    if (it_version == versionMap.end()) {
        sendError(EHEADER); // TODO: switch to other error
        return false;
    }

    path httpPath(req->path);

    int r;
    switch (it_method->second) {
    case HttpMethod::GET:
        if (!exists(httpPath)) return false;

        r = handleGet(httpPath);
        break;
    case HttpMethod::POST:
        // r = handlePost();
        break;
    case HttpMethod::PUT:
        break;
    }

    return false;
}

bool Server::handleGet(path &path) const {
    constexpr char OK_MSG[] = "HTTP/1.1 200 OK\r\n";
    constexpr char CNT_LNG[] = "Content-Length: ";

    constexpr char body[] =
        R"(<!DOCTYPE html>
<html>
    <head>
	<title>Hello World Page Title</title>
    </head>
    <body>
	<h1>Hello, World!</h1>
	<p>This is my first paragraph.</p>
    </body>
</html>)";

    constexpr int size = sizeof(body) - 1;
    std::string resp = std::string(OK_MSG) + std::string(CNT_LNG) +
                       std::to_string(size) + std::string("\r\n") +
                       std::string("\r\n") + std::string(body);

    socket.Send(resp);

    /* test */

    return true;
}
