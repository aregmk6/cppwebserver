#ifndef SERVER_H
#define SERVER_H

#include "amk_socket.h"
#include "threadPool.h"

#include <filesystem>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unordered_map>

enum class HttpMethod { GET, POST, PUT };
enum class HttpVersion { LEGACY, ONE, TWO, THREE };
enum class fileType { HTML, CSS, JS, PNG, JPEG, JPG };

namespace amk
{

class Server
{
public:
  Socket socket;
  std::string buff;

  Server();

private:
  ThreadPool m_tpool;
};

} // namespace amk

#endif
