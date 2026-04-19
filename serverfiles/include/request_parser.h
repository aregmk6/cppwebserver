#ifndef REQ_PARSER_H
#define REQ_PARSER_H

#include "amk_socket.h"
#include "request.h"

#include <string>
#include <strings.h>

namespace amk
{

class ReqParser
{
public:
  enum parse_res { ParsingCompleted, ParsingIncompleted, ParsingError };

  Request parse(const std::string &req_str);
  parse_res check_result() const;

private:
  static bool checkIfConnection(const Request::HeaderPair &item);

  parse_res consume(Request &req, const char *begin, const char *end);

  // Check if a byte is an HTTP character.
  bool isChar(int c);

  // Check if a byte is an HTTP control character.
  bool isControl(int c);

  // Check if a byte is defined as an HTTP special character.
  bool isSpecial(int c);

  // Check if a byte is a digit.
  bool isDigit(int c);

  // The current state of the parser.
  enum State {
    RequestMethodStart,
    RequestMethod,
    RequestUriStart,
    RequestUri,
    RequestHttpVersion_h,
    RequestHttpVersion_ht,
    RequestHttpVersion_htt,
    RequestHttpVersion_http,
    RequestHttpVersion_slash,
    RequestHttpVersion_majorStart,
    RequestHttpVersion_major,
    RequestHttpVersion_minorStart,
    RequestHttpVersion_minor,

    ResponseStatusStart,
    ResponseHttpVersion_ht,
    ResponseHttpVersion_htt,
    ResponseHttpVersion_http,
    ResponseHttpVersion_slash,
    ResponseHttpVersion_majorStart,
    ResponseHttpVersion_major,
    ResponseHttpVersion_minorStart,
    ResponseHttpVersion_minor,
    ResponseHttpVersion_spaceAfterVersion,
    ResponseHttpVersion_statusCodeStart,
    ResponseHttpVersion_spaceAfterStatusCode,
    ResponseHttpVersion_statusTextStart,
    ResponseHttpVersion_newLine,

    HeaderLineStart,
    HeaderLws,
    HeaderName,
    SpaceBeforeHeaderValue,
    HeaderValue,
    ExpectingNewline_2,
    ExpectingNewline_3,

    Post,
    ChunkSize,
    ChunkExtensionName,
    ChunkExtensionValue,
    ChunkSizeNewLine,
    ChunkSizeNewLine_2,
    ChunkSizeNewLine_3,
    ChunkTrailerName,
    ChunkTrailerValue,

    ChunkDataNewLine_1,
    ChunkDataNewLine_2,
    ChunkData
  } state;

  parse_res m_cur_result = ParsingIncompleted;

  int m_body_size    = 0;
  int m_chunk_size   = 0;
  bool m_is_chuncked = false;
  std::string chunkSizeStr;

  ClientSocket m_client_socket;
};

} // namespace amk

#endif
