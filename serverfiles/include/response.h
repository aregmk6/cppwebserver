#ifndef RESPONSE_H
#define RESPONSE_H

#include "file.h"
#include "utils.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace amk {

using std::filesystem::path;

class Response {
public:
  Response() = default;
  bool attach_file(File &&file_) {
    file = std::move(file_);

    std::stringstream res;
    std::cout << "extention: " << file.get_extension() << std::endl;
    auto extension_itr = file_type_map.find(file.get_extension().c_str());
    if (extension_itr == file_type_map.end()) {
      std::cerr << "extention not supported" << std::endl;

      return false;
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

    return true;
  }

  const std::string &header() const;
  const File &get_file() const { return file; }

private:
  std::string m_final_header;
  std::string m_final_body;
  bool m_body_is_file = false;
  File file;

  enum http_method { GET, POST, PUT };
  enum http_version { LEGACY, ONE, TWO, THREE };
  enum file_type { HTML, CSS, JS, PNG, JPEG, JPG };

  static constexpr char ok_msg[] = "HTTP/1.1 200 OK";
  static constexpr char cnt_len[] = "Content-Length:";
  static constexpr char cnt_type[] = "Content-Type:";
  static constexpr char html[] = "text/html; charset=UTF-8";
  static constexpr char css[] = "text/css; charset=UTF-8";
  static constexpr char js[] = "text/javascript";
  static constexpr char png[] = "image/png";
  static constexpr char jpeg[] = "image/jpeg";
  static constexpr char jpg[] = "image/jpg";

  static const std::unordered_map<std::string_view, http_method> method_map;
  static const std::unordered_map<std::string_view, http_version> version_map;
  static const std::unordered_map<std::string_view, file_type> file_type_map;
};

} // namespace amk

#endif
