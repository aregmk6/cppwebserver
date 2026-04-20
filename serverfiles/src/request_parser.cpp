#include "request_parser.h"

#include <algorithm>
#include <cctype>
#include <ctype.h>

using namespace amk;

// TODO: change strcasecmp to system indepednent
// TODO: check if strtol is system depedndent
// TODO: make isDigit, isControl, isSpecial, ... into macros

amk::Request amk::ReqParser::parse(const std::string &req_str)
{
  Request new_req;
  m_cur_result =
      consume(new_req, req_str.c_str(), req_str.c_str() + req_str.size());
  return new_req;
}

ReqParser::parse_res ReqParser::check_result() const
{
  return m_cur_result;
}

bool amk::ReqParser::checkIfConnection(const Request::HeaderPair &item)
{
  std::string lower = item.m_name;
  for (auto &ch : lower) {
    ch = std::tolower(ch);
  }
  return lower.compare("Connection") == 0;
}

amk::ReqParser::parse_res
amk::ReqParser::consume(Request &req, const char *begin, const char *end)
{
  while (begin != end) {
    char input = *begin++;

    switch (state) {
    case RequestMethodStart:
      if (!isChar(input) || isControl(input) || isSpecial(input)) {
        return ParsingError;
      } else {
        state = RequestMethod;
        req.m_method.push_back(input);
      }
      break;
    case RequestMethod:
      if (input == ' ') {
        state = RequestUriStart;
      } else if (!isChar(input) || isControl(input) || isSpecial(input)) {
        return ParsingError;
      } else {
        req.m_method.push_back(input);
      }
      break;
    case RequestUriStart:
      if (isControl(input)) {
        return ParsingError;
      } else {
        state = RequestUri;
        req.m_uri.push_back(input);
      }
      break;
    case RequestUri:
      if (input == ' ') {
        state = RequestHttpVersion_h;
      } else if (input == '\r') {
        req.m_ver_major = 0;
        req.m_ver_minor = 9;

        return ParsingCompleted;
      } else if (isControl(input)) {
        return ParsingError;
      } else {
        req.m_uri.push_back(input);
      }
      break;
    case RequestHttpVersion_h:
      if (input == 'H') {
        state = RequestHttpVersion_ht;
      } else {
        return ParsingError;
      }
      break;
    case RequestHttpVersion_ht:
      if (input == 'T') {
        state = RequestHttpVersion_htt;
      } else {
        return ParsingError;
      }
      break;
    case RequestHttpVersion_htt:
      if (input == 'T') {
        state = RequestHttpVersion_http;
      } else {
        return ParsingError;
      }
      break;
    case RequestHttpVersion_http:
      if (input == 'P') {
        state = RequestHttpVersion_slash;
      } else {
        return ParsingError;
      }
      break;
    case RequestHttpVersion_slash:
      if (input == '/') {
        req.m_ver_major = 0;
        req.m_ver_minor = 0;
        state           = RequestHttpVersion_majorStart;
      } else {
        return ParsingError;
      }
      break;
    case RequestHttpVersion_majorStart:
      if (isDigit(input)) {
        req.m_ver_major = input - '0';
        state           = RequestHttpVersion_major;
      } else {
        return ParsingError;
      }
      break;
    case RequestHttpVersion_major:
      if (input == '.') {
        state = RequestHttpVersion_minorStart;
      } else if (isDigit(input)) {
        req.m_ver_major = req.m_ver_major * 10 + input - '0';
      } else {
        return ParsingError;
      }
      break;
    case RequestHttpVersion_minorStart:
      if (isDigit(input)) {
        req.m_ver_minor = input - '0';
        state           = RequestHttpVersion_minor;
      } else {
        return ParsingError;
      }
      break;
    case RequestHttpVersion_minor:
      if (input == '\r') {
        state = ResponseHttpVersion_newLine;
      } else if (isDigit(input)) {
        req.m_ver_minor = req.m_ver_minor * 10 + input - '0';
      } else {
        return ParsingError;
      }
      break;
    case ResponseHttpVersion_newLine:
      if (input == '\n') {
        state = HeaderLineStart;
      } else {
        return ParsingError;
      }
      break;
    case HeaderLineStart:
      if (input == '\r') {
        state = ExpectingNewline_3;
      } else if (!req.m_headers.empty() && (input == ' ' || input == '\t')) {
        state = HeaderLws;
      } else if (!isChar(input) || isControl(input) || isSpecial(input)) {
        return ParsingError;
      } else {
        req.m_headers.push_back(Request::HeaderPair());
        req.m_headers.back().m_name.reserve(16);
        req.m_headers.back().m_value.reserve(16);
        req.m_headers.back().m_name.push_back(input);
        state = HeaderName;
      }
      break;
    case HeaderLws:
      if (input == '\r') {
        state = ExpectingNewline_2;
      } else if (input == ' ' || input == '\t') {
      } else if (isControl(input)) {
        return ParsingError;
      } else {
        state = HeaderValue;
        req.m_headers.back().m_value.push_back(input);
      }
      break;
    case HeaderName:
      if (input == ':') {
        state = SpaceBeforeHeaderValue;
      } else if (!isChar(input) || isControl(input) || isSpecial(input)) {
        return ParsingError;
      } else {
        req.m_headers.back().m_name.push_back(input);
      }
      break;
    case SpaceBeforeHeaderValue:
      if (input == ' ') {
        state = HeaderValue;
      } else {
        return ParsingError;
      }
      break;
    case HeaderValue:
      if (input == '\r') {
        if (req.m_method == "POST" || req.m_method == "PUT") {
          Request::HeaderPair &h = req.m_headers.back();

          if (strcasecmp(h.m_name.c_str(), "Content-Length") == 0) {
            m_body_size = atoi(h.m_value.c_str());
            req.m_body.reserve(m_body_size);
          } else if (strcasecmp(h.m_name.c_str(), "Transfer-Encoding") == 0) {
            if (strcasecmp(h.m_value.c_str(), "chunked") == 0)
              m_is_chuncked = true;
          }
        }
        state = ExpectingNewline_2;
      } else if (isControl(input)) {
        return ParsingError;
      } else {
        req.m_headers.back().m_value.push_back(input);
      }
      break;
    case ExpectingNewline_2:
      if (input == '\n') {
        state = HeaderLineStart;
      } else {
        return ParsingError;
      }
      break;
    case ExpectingNewline_3: {
      auto it = std::find_if(
          req.m_headers.begin(), req.m_headers.end(), checkIfConnection);

      if (it != req.m_headers.end()) {
        if (strcasecmp(it->m_value.c_str(), "Keep-Alive") == 0) {
          req.m_keep_alive = true;
        } else // == Close
        {
          req.m_keep_alive = false;
        }
      } else {
        if (req.m_ver_major > 1 ||
            (req.m_ver_major == 1 && req.m_ver_minor == 1))
          req.m_keep_alive = true;
      }

      if (m_is_chuncked) {
        state = ChunkSize;
      } else if (m_body_size == 0) {
        if (input == '\n')
          return ParsingCompleted;
        else
          return ParsingError;
      } else {
        state = Post;
      }
      break;
    }
    case Post:
      --m_body_size;
      req.m_body.push_back(input);

      if (m_body_size == 0) {
        return ParsingCompleted;
      }
      break;
    case ChunkSize:
      if (isalnum(input)) {
        chunkSizeStr.push_back(input);
      } else if (input == ';') {
        state = ChunkExtensionName;
      } else if (input == '\r') {
        state = ChunkSizeNewLine;
      } else {
        return ParsingError;
      }
      break;
    case ChunkExtensionName:
      if (isalnum(input) || input == ' ') {
        // skip
      } else if (input == '=') {
        state = ChunkExtensionValue;
      } else if (input == '\r') {
        state = ChunkSizeNewLine;
      } else {
        return ParsingError;
      }
      break;
    case ChunkExtensionValue:
      if (isalnum(input) || input == ' ') {
        // skip
      } else if (input == '\r') {
        state = ChunkSizeNewLine;
      } else {
        return ParsingError;
      }
      break;
    case ChunkSizeNewLine:
      if (input == '\n') {
        m_body_size = strtol(chunkSizeStr.c_str(), NULL, 16);
        chunkSizeStr.clear();
        req.m_body.reserve(req.m_body.size() + m_chunk_size);

        if (m_chunk_size == 0)
          state = ChunkSizeNewLine_2;
        else
          state = ChunkData;
      } else {
        return ParsingError;
      }
      break;
    case ChunkSizeNewLine_2:
      if (input == '\r') {
        state = ChunkSizeNewLine_3;
      } else if (isalpha(input)) {
        state = ChunkTrailerName;
      } else {
        return ParsingError;
      }
      break;
    case ChunkSizeNewLine_3:
      if (input == '\n') {
        return ParsingCompleted;
      } else {
        return ParsingError;
      }
      break;
    case ChunkTrailerName:
      if (isalnum(input)) {
        // skip
      } else if (input == ':') {
        state = ChunkTrailerValue;
      } else {
        return ParsingError;
      }
      break;
    case ChunkTrailerValue:
      if (isalnum(input) || input == ' ') {
        // skip
      } else if (input == '\r') {
        state = ChunkSizeNewLine;
      } else {
        return ParsingError;
      }
      break;
    case ChunkData:
      req.m_body.push_back(input);

      if (--m_chunk_size == 0) {
        state = ChunkDataNewLine_1;
      }
      break;
    case ChunkDataNewLine_1:
      if (input == '\r') {
        state = ChunkDataNewLine_2;
      } else {
        return ParsingError;
      }
      break;
    case ChunkDataNewLine_2:
      if (input == '\n') {
        state = ChunkSize;
      } else {
        return ParsingError;
      }
      break;
    default:
      return ParsingError;
    }
  }

  return ParsingIncompleted;
}
bool amk::ReqParser::isChar(int c)
{
  return c >= 0 && c <= 127;
}
bool amk::ReqParser::isControl(int c)
{
  return (c >= 0 && c <= 31) || (c == 127);
}
bool amk::ReqParser::isSpecial(int c)
{
  switch (c) {
  case '(':
  case ')':
  case '<':
  case '>':
  case '@':
  case ',':
  case ';':
  case ':':
  case '\\':
  case '"':
  case '/':
  case '[':
  case ']':
  case '?':
  case '=':
  case '{':
  case '}':
  case ' ':
  case '\t':
    return true;
  default:
    return false;
  }
}
bool amk::ReqParser::isDigit(int c)
{
  return c >= '0' && c <= '9';
}
