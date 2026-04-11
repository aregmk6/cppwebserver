#ifndef RESPONSE_H
#define RESPONSE_H

#include "file.h"
#include "utils.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace amk
{

using std::filesystem::path;

class Response
{
public:
  Response(const amk::File &file);

  const std::string &header() const;

private:
  std::string final_header;
  std::string final_body;

  enum http_method { GET, POST, PUT };
  enum http_version { LEGACY, ONE, TWO, THREE };
  enum file_type { HTML, CSS, JS, PNG, JPEG, JPG };

  static constexpr char ok_msg[]   = "HTTP/1.1 200 OK";
  static constexpr char cnt_len[]  = "Content-Length:";
  static constexpr char cnt_type[] = "Content-Type:";
  static constexpr char html[]     = "text/html; charset=UTF-8";
  static constexpr char css[]      = "text/css; charset=UTF-8";
  static constexpr char js[]       = "text/javascript";
  static constexpr char png[]      = "image/png";
  static constexpr char jpeg[]     = "image/jpeg";
  static constexpr char jpg[]      = "image/jpg";

  static const std::unordered_map<std::string_view, http_method> method_map;
  static const std::unordered_map<std::string_view, http_version> version_map;
  static const std::unordered_map<std::string_view, file_type> file_type_map;
};

} // namespace amk

#endif
