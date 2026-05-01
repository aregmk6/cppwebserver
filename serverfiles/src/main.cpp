#include "mysocket.h"
#include "server.h"
#include "utils.h"

#include <unistd.h>

using namespace amk;

int main() {
  Logger logger{"master"};
  Server &server = Server::create_instance(6969);

  while (true) {
    logger.log("waiting for connection...");

    ClientSocket client_conn = server.wait_for_connection();

    if (server.should_stop())
      break;

    logger.log("accepted");

    server.send_client(client_conn);

    logger.log("sent off");
  };

  server.shutdown();
}
