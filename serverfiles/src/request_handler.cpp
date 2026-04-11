#include "request_handler.h"

#include "file.h"

using namespace amk;

void ReqHandler::handle_get(path &uri)
{
  static constexpr char OK_MSG[]   = "HTTP/1.1 200 OK\r\n";
  static constexpr char CNT_LNG[]  = "Content-Length: ";
  static constexpr char CNT_TYPE[] = "Content-Type: ";
  static std::string FNL_LNG;
  static std::string FNL_TYPE;

  amk::File file{uri, File::open_mode::read};

  auto it_exten = fileTypeMap.find(uri.extension().c_str());
  if (it_exten == fileTypeMap.end()) {
    std::cerr << "extention not supported" << std::endl;
    return false;
  }

  switch (it_exten->second) {
  case file_type::HTML:
    FNL_TYPE = CNT_TYPE + std::string("text/html; charset=UTF-8") + delim;
    break;
  case file_type::CSS:
    FNL_TYPE = CNT_TYPE + std::string("text/css; charset=UTF-8") + delim;
    break;
  case file_type::JS:
    FNL_TYPE = CNT_TYPE + std::string("text/javascript") + delim;
    break;
  case file_type::PNG:
    FNL_TYPE = CNT_TYPE + std::string("image/png") + delim;
    break;
  case file_type::JPEG:
    FNL_TYPE = CNT_TYPE + std::string("image/jpeg") + delim;
    break;
  case file_type::JPG:
    FNL_TYPE = CNT_TYPE + std::string("image/jpg") + delim;
    break;
  }

  FNL_LNG = CNT_LNG + std::to_string(file.get_size()) + delim;
  buff    = OK_MSG + FNL_LNG + FNL_TYPE + delim;

  socket.cork();

  socket.Send(buff);
  socket.SendFile(file.fd(), file.get_size());

  socket.uncork();

  /* test */
  return true;
}
