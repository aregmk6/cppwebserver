#include "amk_socket.h"
#include "server.h"

#include <chrono>
#include <iostream>
#include <unistd.h>

using namespace amk;

using Clock     = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

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
