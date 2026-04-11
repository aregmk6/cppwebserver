#include "file.h"

using namespace amk;

file::file(path src, open_mode mode)
{
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

  calc_file_size();
}
amk::file::~file()
{
  close(file_descriptor);
}
size_t amk::file::get_size() const
{
  return file_size;
}
int amk::file::fd() const
{
  return file_descriptor;
}

void amk::file::calc_file_size()
{
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
