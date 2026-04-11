#include "amk_socket.h"
#include "server.h"

#include <iostream>
#include <unistd.h>

using namespace amk;

int main()
{
  Server server;

  while (true) {
    ClientSocket client_conn = server.wait_for_connection();

    std::cout << "accepted" << std::endl;

    server.send_client(client_conn);

    std::cout << "sent off" << std::endl;
  };
}
