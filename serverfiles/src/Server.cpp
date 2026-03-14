#include "Server.h"
#include "consts.h"
#include <cstddef>
#include <iostream>
#include <string_view>

Server::Server() : socket(), buff() {
    buff.resize(MAX_HEADER_SIZE + 1, 0);
}

bool Server::handle() {
    int headerSize = socket.readHeader(buff);
    if (headerSize < 0) {
        std::cerr << "Read Failed" << std::endl;
    }

    std::string_view window(buff.c_str());
    std::string_view curLine;
    size_t lineEnd = std::string::npos;
    size_t wordEnd = std::string::npos;
    Server::method reqMethod;

    /* handle first line */
    lineEnd = window.find(DELIM);
    if (lineEnd == std::string::npos) {
        sendError(EHEADER);
        return false;
    }

    curLine = window.substr(0, lineEnd);
    while (1) {
        wordEnd = curLine.find(SPACE);
        if (wordEnd == std::string::npos) {
            sendError(EHEADER);
            return false;
        }
    }

    /* handle rest of the lines */
    while (lineEnd) {
        lineEnd = window.find(DELIM);
        if (lineEnd == std::string::npos) {
            sendError(EHEADER);
            return false;
        }

        curLine = window.substr(0, lineEnd);

        // parse line

        window.remove_prefix(lineEnd + 2); // 2 = size of "\r\n"
    }

    switch (reqMethod) {
    case GET:
        break;
    case POST:
        break;
    case PUT:
        break;
    }
}
