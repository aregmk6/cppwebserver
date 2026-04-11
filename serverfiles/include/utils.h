#ifndef UTILS_H
#define UTILS_H

namespace amk
{

constexpr char HEADER_END[] = "\r\n\r\n";
constexpr char DELIM[]      = "\r\n";
constexpr char SPACE        = ' ';

constexpr int MAX_HEADER_SIZE        = 1024 * 8;
constexpr int FIRST_LINE_HEADER_SIZE = 3;

// errors
constexpr int EHEADER = 400;

} // namespace amk

#endif
