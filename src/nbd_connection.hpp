#ifndef NBD_CONNECTION_HPP
#define NBD_CONNECTION_HPP

#include <liburing.h>
#include <string>
class NbdConnection {
  int socket_fd;
  int size;
  io_uring *ring_ptr;

  std::string name;

public:
  /**
   * Creates a socket
   */
  NbdConnection(const std::string &&s, const int port, io_uring *ring);
  ~NbdConnection();

  int get_socket() { return socket_fd; };
  int get_size() { return size; };
};

#endif // NBD_CONNECTION_HPP
