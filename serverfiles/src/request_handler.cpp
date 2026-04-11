#include "request_handler.h"

#include "file.h"

using namespace amk;

void ReqHandler::handle_get(path &uri)
{
  File src_file(uri, File::open_mode::read);

  m_client_socket.send_response(src_file);
}
