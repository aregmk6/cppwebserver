#ifndef REQUEST_H
#define REQUEST_H

#include "utils.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace amk
{

class Request
{
public:
  struct HeaderPair {
    std::string m_name;
    std::string m_value;
  };

  std::string get_req() const;
  bool is_keep_alive() const
  {
    return m_keep_alive;
  }
  bool is_valid() const
  {
    return m_valid;
  }
  void invalidate()
  {
    m_valid = false;
  }

  std::string m_method;
  std::string m_uri;
  int m_ver_major = 0;
  int m_ver_minor = 0;
  std::vector<HeaderPair> m_headers;
  std::string m_body;
  bool m_keep_alive = false;

  bool m_valid = false;
};

} // namespace amk

#endif
