#include "file.h"
#include <iostream>

using namespace amk;

File::File(const path &src, open_mode mode) {
  file_path = src;

  std::cout << "the opened file has the path: " << file_path << std::endl;

  switch (mode) {
  case open_mode::read:
    file_descriptor = open(src.c_str(), O_RDONLY);
    break;
  case open_mode::write:
    file_descriptor = open(src.c_str(), O_WRONLY);
    break;
  case open_mode::read_write:
    file_descriptor = open(src.c_str(), O_RDWR);
    break;
  }

  if (file_descriptor < 0) {
    perror("open");
    exit(EXIT_FAILURE);
  }

  std::cout << "fd number " << file_descriptor << " opened" << std::endl;

  calc_file_size();
}
amk::File::~File() {
  if (file_descriptor != -1)
    std::cout << "fd number " << file_descriptor << " is closing" << std::endl;
  close(file_descriptor);
}
size_t amk::File::get_size() const { return file_size; }
const amk::path amk::File::get_extension() const {
  return file_path.extension();
}

int amk::File::fd() const { return file_descriptor; }

void amk::File::calc_file_size() {
  file_size = lseek(file_descriptor, 0, SEEK_END);
  if (file_size < 0) {
    perror("lseek end");
    exit(EXIT_FAILURE);
  }

  if (lseek(file_descriptor, 0, SEEK_SET) < 0) {
    perror("lseek set");
    exit(EXIT_FAILURE);
  }
}
