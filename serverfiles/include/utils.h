#ifndef UTILS_H
#define UTILS_H

namespace amk
{

constexpr char header_end[] = "\r\n\r\n";
constexpr char delim[]      = "\r\n";
constexpr char space        = ' ';

constexpr int max_header_size        = 1024 * 8;
constexpr int first_line_header_size = 3;

// errors
constexpr int eheader = 400;

} // namespace amk

#endif
