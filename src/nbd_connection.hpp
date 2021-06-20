#ifndef NBD_CONNECTION_HPP
#define NBD_CONNECTION_HPP

class NbdConnection {
  int socket_fd;
  int size;

public:
  /**
   * Creates a socket
   */
  NbdConnection(const int port);
  ~NbdConnection();

  int get_socket() { return socket_fd; };
  int get_size() { return size; };

  int submit_read_request();
};

#endif // NBD_CONNECTION_HPP
