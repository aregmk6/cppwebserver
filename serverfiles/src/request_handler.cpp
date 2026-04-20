#include "request_handler.h"

#include "file.h"
#include "request.h"

using namespace amk;

void ReqHandler::handle_conn(ClientSocket &new_conn)
{
  m_client_socket = new_conn;
  handle_conn();
}

// TODO: add keep alive support (need to check how to handle that)
void ReqHandler::handle_conn()
{
  parse();
  if (m_cur_req.is_valid()) {
    handle(m_cur_req);
  }
}

void ReqHandler::parse()
{
  m_client_socket.read_header();
  m_cur_req = m_parser.parse(m_client_socket.get_buf());
  if (m_parser.check_result() == ReqParser::parse_res::ParsingError) {
    m_cur_req.invalidate();
    std::cerr << "parser error" << std::endl;
    return;
  }
}

// TODO: change the methods to enums to support cases.
void ReqHandler::handle(const Request &req)
{
  if (req.m_method == "GET") {
    handle_get(req.m_uri);
  } // else if (POST) {
    // ...
    // } else ...
}

void ReqHandler::handle_get(const path &uri)
{
  File src_file(uri, File::open_mode::read);
  Response res(src_file);

  m_client_socket.send_response(res, src_file);
}
