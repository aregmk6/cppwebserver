#include "response.h"

using namespace amk;

const std::unordered_map<std::string_view, Response::http_method>
    Response::method_map = {
        { "GET",  GET},
        {"POST", POST},
        { "PUT",  PUT}
};

const std::unordered_map<std::string_view, Response::http_version>
    Response::version_map = {
        {"HTTP/1.0", LEGACY},
        {"HTTP/1.1",    ONE},
        {  "HTTP/2",    TWO},
        {  "HTTP/3",  THREE}
};

const std::unordered_map<std::string_view, Response::file_type>
    Response::file_type_map = {
        {".html", HTML},
        { ".css",  CSS},
        {  ".js",   JS},
        { ".png",  PNG},
        {".jpeg", JPEG},
        { ".jpg",  JPG}
};

amk::Response::Response(const amk::File &file)
{
  std::stringstream res;
  auto extension_itr = file_type_map.find(file.get_extension().c_str());
  if (extension_itr == file_type_map.end()) {
    std::cerr << "extention not supported" << std::endl;
  }
  m_body_is_file = true;

  res << ok_msg << delim                             //
      << cnt_len << ' ' << file.get_size() << delim; //

  switch (extension_itr->second) {
  case file_type::HTML:
    res << cnt_type << ' ' << html << delim;
    break;
  case file_type::CSS:
    res << cnt_type << ' ' << css << delim;
    break;
  case file_type::JS:
    res << cnt_type << ' ' << js << delim;
    break;
  case file_type::PNG:
    res << cnt_type << ' ' << png << delim;
    break;
  case file_type::JPEG:
    res << cnt_type << ' ' << jpeg << delim;
    break;
  case file_type::JPG:
    res << cnt_type << ' ' << jpg << delim;
    break;
  }

  res << delim;
  m_final_header = res.str();
  // body will be sent with zero copy by the socket
}
const std::string &amk::Response::header() const
{
  return m_final_header;
}
