#include "response.h"

using namespace amk;

const std::unordered_map<std::string_view, Response::http_method>
    Response::method_map = {{"GET", GET}, {"POST", POST}, {"PUT", PUT}};

const std::unordered_map<std::string_view, Response::http_version>
    Response::version_map = {{"HTTP/1.0", LEGACY},
                             {"HTTP/1.1", ONE},
                             {"HTTP/2", TWO},
                             {"HTTP/3", THREE}};

const std::unordered_map<std::string_view, Response::file_type>
    Response::file_type_map = {{".html", HTML}, {".css", CSS},   {".js", JS},
                               {".png", PNG},   {".jpeg", JPEG}, {".jpg", JPG}};

const std::string &Response::header() const { return m_final_header; }
