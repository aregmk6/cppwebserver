#include "response.h"

using namespace amk;

const std::unordered_map<std::string_view, Response::http_method>
    Response::methodMap = {
        { "GET",  GET},
        {"POST", POST},
        { "PUT",  PUT}
};

const std::unordered_map<std::string_view, Response::http_version>
    Response::versionMap = {
        {"HTTP/1.0", LEGACY},
        {"HTTP/1.1",    ONE},
        {  "HTTP/2",    TWO},
        {  "HTTP/3",  THREE}
};

const std::unordered_map<std::string_view, Response::file_type>
    Response::fileTypeMap = {
        {".html", HTML},
        { ".css",  CSS},
        {  ".js",   JS},
        { ".png",  PNG},
        {".jpeg", JPEG},
        { ".jpg",  JPG}
};
