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

  int submit_read_request(u_int64_t handle, u_int64_t offset, u_int32_t n_bytes);
};

#endif // NBD_CONNECTION_HPP
