#include "server.h"

#include "file.h"
#include "utils.h"

#include <cstddef>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <sys/sendfile.h>
#include <unistd.h>
#include <unordered_map>

using namespace amk;

static const std::unordered_map<std::string_view, HttpMethod> methodMap = {
    { "GET",  HttpMethod::GET},
    {"POST", HttpMethod::POST},
    { "PUT",  HttpMethod::PUT}
};

static const std::unordered_map<std::string_view, HttpVersion> versionMap = {
    {"HTTP/1.0", HttpVersion::LEGACY},
    {"HTTP/1.1",    HttpVersion::ONE},
    {  "HTTP/2",    HttpVersion::TWO},
    {  "HTTP/3",  HttpVersion::THREE}
};

static const std::unordered_map<std::string_view, fileType> fileTypeMap = {
    {".html", fileType::HTML},
    { ".css",  fileType::CSS},
    {  ".js",   fileType::JS},
    { ".png",  fileType::PNG},
    {".jpeg", fileType::JPEG},
    { ".jpg",  fileType::JPG}
};

server::server() : buff(MAX_HEADER_SIZE + 1, 0) {}

void server::sendError(int num) const
{
  return;
}

bool server::parse()
{
  int headerSize = socket.readHeader(buff);
  if (headerSize < 0) {
    std::cerr << "Read Failed" << std::endl;
  }

  std::string_view window(buff.c_str());
  std::string_view firstLine;
  std::string_view curLine;
  std::string_view curWord;
  size_t lineEnd = std::string::npos;
  size_t wordEnd = std::string::npos;
  auto req       = std::make_unique<HttpRequest>();

  /* handle first line */
  lineEnd = window.find(DELIM);
  if (lineEnd == std::string::npos) {
    sendError(EHEADER);
    return false;
  }

  firstLine = window.substr(0, lineEnd);
  window.remove_prefix(lineEnd + 2);
  int count = 0;
  while (count < FIRST_LINE_HEADER_SIZE) {
    if (count != FIRST_LINE_HEADER_SIZE - 1) {
      wordEnd = firstLine.find(SPACE);

      if (wordEnd == std::string::npos) {
        sendError(EHEADER);
        return false;
      }

      curWord = firstLine.substr(0, wordEnd);
    } else {
      curWord = firstLine;
      wordEnd = -1;
    }

    switch (count) {
    case 0:
      req->method = curWord;
      break;
    case 1:
      req->path = curWord;
      break;
    case 2:
      req->version = curWord;
      break;
    }

    firstLine.remove_prefix(wordEnd + 1); // size of a single space = 1

    count++;
  }

  /*
      GET /index.html HTTP/1.1\r\n
      Host: aregm.com\r\n
      User-Agent: Mozilla/5.0 (Arch Linux)\r\n
      Accept: text/html,application/xhtml+xml\r\n
      Accept-Language: en-US,en;q=0.9\r\n
      Connection: keep-alive\r\n
      \r\n
      [Optional Body Data Starts Here]
  */

  /* handle rest of the lines */
  size_t colon = 0;
  std::string_view key, value;
  while (1) {
    lineEnd = window.find(DELIM);

    if (lineEnd == std::string::npos) {
      sendError(EHEADER);
      return false;
    }

    if (!lineEnd)
      break;

    curLine = window.substr(0, lineEnd);

    colon = curLine.find(':');
    if (colon == std::string::npos) {
      sendError(EHEADER);
      return false;
    }

    key   = curLine.substr(0, colon);
    value = curLine.substr(colon + 1, lineEnd - colon - 1);

    req->headers.insert({key, value});

    window.remove_prefix(lineEnd + 2); // size of "\r\n" = 2
  }

  // TODO: add body parser

  return handle(std::move(req));
}

using namespace std::filesystem;

bool server::handle(std::unique_ptr<HttpRequest> req)
{
  auto it_method = methodMap.find(req->method);
  if (it_method == methodMap.end()) {
    sendError(EHEADER); // TODO: switch to other error
    return false;
  }

  auto it_version = versionMap.find(req->version);
  if (it_version == versionMap.end()) {
    sendError(EHEADER); // TODO: switch to other error
    return false;
  }

  static const path rootPath = "/home/aregmk/Coding/web/cppserver/public/";

  path httpPath(req->path);
  path workPath;

  for (const auto &path_part : httpPath) {
    if (path_part == "..") {
      sendError(EHEADER); // TODO: switch to other error
      return false;
    }
  }

  if (httpPath == "/") {
    workPath = rootPath / "index.html";
  } else {
    workPath = rootPath / httpPath.relative_path();
  }

  std::cout << httpPath << std::endl;
  std::cout << workPath << std::endl;

  int r;
  switch (it_method->second) {
  case HttpMethod::GET:
    if (!exists(workPath))
      return false;

    r = handleGet(workPath);
    break;
  case HttpMethod::POST:
    // r = handlePost();
    break;
  case HttpMethod::PUT:
    break;
  }

  return r;
}

bool server::handleGet(path &workPath)
{
  // TODO: add more content types

  static constexpr char OK_MSG[]   = "HTTP/1.1 200 OK\r\n";
  static constexpr char CNT_LNG[]  = "Content-Length: ";
  static constexpr char CNT_TYPE[] = "Content-Type: ";
  static std::string FNL_LNG;
  static std::string FNL_TYPE;

  amk::file file{workPath, file::open_mode::read};

  auto it_exten = fileTypeMap.find(workPath.extension().c_str());
  if (it_exten == fileTypeMap.end()) {
    std::cerr << "extention not supported" << std::endl;
    return false;
  }

  switch (it_exten->second) {
  case fileType::HTML:
    FNL_TYPE = CNT_TYPE + std::string("text/html; charset=UTF-8") + DELIM;
    break;
  case fileType::CSS:
    FNL_TYPE = CNT_TYPE + std::string("text/css; charset=UTF-8") + DELIM;
    break;
  case fileType::JS:
    FNL_TYPE = CNT_TYPE + std::string("text/javascript") + DELIM;
    break;
  case fileType::PNG:
    FNL_TYPE = CNT_TYPE + std::string("image/png") + DELIM;
    break;
  case fileType::JPEG:
    FNL_TYPE = CNT_TYPE + std::string("image/jpeg") + DELIM;
    break;
  case fileType::JPG:
    FNL_TYPE = CNT_TYPE + std::string("image/jpg") + DELIM;
    break;
  }

  FNL_LNG = CNT_LNG + std::to_string(file.get_size()) + DELIM;
  buff    = OK_MSG + FNL_LNG + FNL_TYPE + DELIM;

  socket.cork();

  socket.Send(buff);
  socket.SendFile(file.fd(), file.get_size());

  socket.uncork();

  /* test */
  return true;
}
