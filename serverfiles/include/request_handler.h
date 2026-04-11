#ifndef REQ_HANDLER_H
#define REQ_HANDLER_H

#include "amk_socket.h"
#include "request.h"

#include <filesystem>

namespace amk
{

using std::filesystem::path;

class ReqHandler
{
public:
  void handle(Request &req);
  void handle_get(path &uri);

private:
  Socket m_socket;
};

} // namespace amk

#endif
