#include "utils.h"

#include <iostream>

using namespace amk;

void Logger::log(const char *str) {
  std::cout << user << ": " << str << std::endl;
}

void Logger::log(const std::string &str) {
  std::cout << user << ": " << str << std::endl;
}
