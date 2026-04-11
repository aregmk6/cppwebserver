#ifndef RESPONSE_H
#define RESPONSE_H

#include <string_view>
#include <unordered_map>

namespace amk
{

class Response
{
public:
  enum http_method { GET, POST, PUT };
  enum http_version { LEGACY, ONE, TWO, THREE };
  enum file_type { HTML, CSS, JS, PNG, JPEG, JPG };

private:
  static const std::unordered_map<std::string_view, http_method> methodMap;
  static const std::unordered_map<std::string_view, http_version> versionMap;
  static const std::unordered_map<std::string_view, file_type> fileTypeMap;
};

} // namespace amk

#endif
