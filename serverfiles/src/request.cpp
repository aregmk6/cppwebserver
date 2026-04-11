#include "request.h"

using namespace amk;

std::string amk::Request::get_req() const
{
  std::stringstream output;
  output << method << ' '                            //
         << uri << ' '                               //
         << "HTTP/" << ver_major << '.' << ver_minor //
         << delim;

  for (const auto &HeaderPair : m_headers) {
    output << HeaderPair.m_name << ": " //
           << HeaderPair.m_value        //
           << delim;
  }

  output << delim;
  output << m_body;

  output << std::endl;

  return output.str();
}

bool Request::is_keep_alive() const
{
  return m_keep_alive;
}
bool amk::Request::is_valid() const
{
  return m_valid;
}
void amk::Request::invalidate()
{
  m_valid = false;
}
