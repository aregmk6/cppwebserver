#include "Socket.h"
#include <cstdio>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>

Socket::Socket() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    serverAddr.addr.sin_family = AF_INET;
    serverAddr.addr.sin_port = htons(8080);
    serverAddr.addr.sin_addr.s_addr = htons(INADDR_LOOPBACK);
    serverAddr.addrLen = sizeof(serverAddr.addr);
}

void Socket::Bind() const {
    int r =
        bind(sockfd, (struct sockaddr *)&serverAddr.addr, serverAddr.addrLen);

    if (r < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
}

void Socket::Accept() {
    clientfd = accept(
        sockfd, (struct sockaddr *)&clientAddr.addr, &clientAddr.addrLen);

    if (clientfd < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
}

void Socket::Listen() const {
    int r = listen(sockfd, 5);

    if (r < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

int Socket::readHeader(std::string &buf) const {
    int totalBytes = 0;
    while (1) {
        int br = recv(sockfd, &buf[0], MAX_HEADER_SIZE, 0);

        totalBytes += br;

        if (buf.find(HEADER_END) != std::string::npos) {
            break;
        }

        if (totalBytes == MAX_HEADER_SIZE) {
            return -1;
        }
    }

    return totalBytes;
}

bool Socket::Send(std::string &buf) const {
    int startFrom = 0;
    int bytesLeft = buf.size();

    while (1) {
        int bs = send(sockfd, buf.c_str() + startFrom, bytesLeft, 0);

        if (bs <= 0) {
            return false;
        }

        if (bytesLeft <= 0) {
            break;
        }

        startFrom += bs;
        bytesLeft -= bs;
    }

    return true;
}
