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

Server::Server() : buff(max_header_size + 1, 0) {}
