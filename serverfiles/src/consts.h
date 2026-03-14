#ifndef CONSTS_H
#define CONSTS_H

constexpr char HEADER_END[] = "\r\n\r\n";
constexpr char DELIM[] = "\r\n";
constexpr char SPACE = ' ';

constexpr int MAX_HEADER_SIZE = 1024 * 8;
constexpr int FL_HEADER_NR = 3;

// errors
constexpr int EHEADER = 400;

#endif
