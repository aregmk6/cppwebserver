#include "src/Server.h"
#include <iostream>
#include <sys/socket.h>

int main() {
    Server server;
    server.socket.Bind();
    server.socket.Listen();

    while (1) {
        server.socket.Accept();
        std::cout << "accepted" << std::endl;

        server.parse();

        std::cout << "handled" << std::endl;

        server.socket.Close();

        std::cout << "connection closed" << std::endl;
    };
}
