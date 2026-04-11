#include "server.h"

#include <chrono>
#include <iostream>

using namespace amk;

using Clock     = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

int main()
{
  server server;
  server.socket.Bind();
  server.socket.Listen();

  auto now = Clock::now();

  while (1) {
    server.socket.Accept();
    std::cout << "accepted" << std::endl;

    while (1) {
      server.parse();

      auto after = Clock::now();
      if (TimePoint(after - now) > TimePoint(std::chrono::seconds(2))) {
        std::cout << "clock ended" << std::endl;
        break;
      }
    }

    std::cout << "handled" << std::endl;

    server.socket.Close();

    std::cout << "connection closed" << std::endl;
  };
}
