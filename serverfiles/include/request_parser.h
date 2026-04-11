#ifndef REQ_PARSER_H
#define REQ_PARSER_H

#include "amk_socket.h"
#include "request.h"

#include <memory>

namespace amk
{

class ReqParser
{
public:
  enum parse_res { success, failure, incomplete };

  Request parse(const char *req_str) const;

private:
  enum cur_state { PLACE_HOLDER };
  parse_res m_cur_result = incomplete;

  int m_body_size    = 0;
  int m_chunk_size   = 0;
  bool m_is_chuncked = false;

  ClientSocket m_client_socket;
};

} // namespace amk

#endif
