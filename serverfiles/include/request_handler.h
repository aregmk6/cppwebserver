#ifndef REQ_HANDLER_H
#define REQ_HANDLER_H

#include "amk_socket.h"
#include "request.h"
#include "request_parser.h"

#include <filesystem>
#include <iostream>

namespace amk
{

using std::filesystem::path;

class ReqHandler
{
public:
  void handle_conn(ClientSocket &new_conn);

  void handle_conn();

private:
  void parse();
  void handle(Request &req);

  void handle_get(path &uri);

  ClientSocket m_client_socket;
  ReqParser m_parser;
  Request m_cur_req;
};

} // namespace amk

#endif
