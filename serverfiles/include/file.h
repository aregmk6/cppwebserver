#ifndef FILE_H
#define FILE_H

#include <fcntl.h>
#include <filesystem>
#include <unistd.h>

namespace amk
{

using std::filesystem::path;

// TODO: add more methods, for now this is all because I don't need more.

class File
{
public:
  enum class open_mode { read, write, read_write };
  File(path src, open_mode mode);
  ~File();
  size_t get_size() const;
  int fd() const;

private:
  int file_descriptor;
  size_t file_size;

  void calc_file_size();
};

} // namespace amk

#endif
