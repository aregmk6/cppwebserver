#ifndef FILE_H
#define FILE_H

#include <fcntl.h>
#include <filesystem>
#include <unistd.h>

namespace amk {

using std::filesystem::path;

// TODO: add more methods, for now this is all because I don't need more.

class File {
public:
  enum class open_mode { read, write, read_write };

  File() : file_size(0), file_descriptor(-1), file_path("") {}
  File(const path &src, open_mode mode = open_mode::read);
  File(const File &) = delete;
  File &operator=(const File &) = delete;
  File(File &&other) {
    file_descriptor = other.file_descriptor;
    other.file_descriptor = -1;

    file_size = other.file_size;
    file_path = other.file_path;
  }
  File &operator=(File &&other) {
    file_descriptor = other.file_descriptor;
    other.file_descriptor = -1;

    file_size = other.file_size;
    file_path = other.file_path;
    return *this;
  }
  ~File();
  size_t get_size() const;
  const path get_extension() const;
  int fd() const;

private:
  int file_descriptor = -1;
  size_t file_size;
  path file_path;

  void calc_file_size();
};

} // namespace amk

#endif
