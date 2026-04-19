#include "request_handler.h"

#include "file.h"

using namespace amk;

void amk::ReqHandler::handle_conn(ClientSocket &new_conn)
{
  m_client_socket = new_conn;
  handle_conn();
}
void amk::ReqHandler::handle_conn()
{
  parse();
  if (m_cur_req.is_valid()) {
    handle(m_cur_req);
  }
}
void amk::ReqHandler::parse()
{
  m_client_socket.read_header();
  m_cur_req = m_parser.parse(m_client_socket.get_buf());
  if (m_parser.check_result() == ReqParser::parse_res::ParsingError) {
    m_cur_req.invalidate();
    std::cerr << "parser error" << std::endl;
    return;
  }
}

void ReqHandler::handle_get(path &uri)
{
  File src_file(uri, File::open_mode::read);

  m_client_socket.send_response(src_file);
}
