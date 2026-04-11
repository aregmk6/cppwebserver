#include "request_parser.h"

using namespace amk;

ReqParser::parse_res ReqParser::check_result() const
{
  return m_cur_result;
}
