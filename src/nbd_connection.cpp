#include "nbd_connection.hpp"
#include "fmt/core.h"
#include "nbd_types.hpp"
#include "oldstyle.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <bits/stdint-uintn.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <endian.h>
#include <liburing.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

NbdConnection::NbdConnection(const std::string &&name, const int port, io_uring *ring)
    : ring_ptr{ring}, name{name} {
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_fd == -1) {
    fprintf(stderr, "Problem opening unix socket");
  }

  struct sockaddr_in addr;
  addr.sin_port = htons(port);
  addr.sin_family = AF_INET;

  if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    exit(1);
  }

  int connection_result = ::connect(socket_fd, (struct sockaddr *)(&addr), sizeof(addr));
  if (connection_result == -1) {
    fprintf(stderr, "Problem connecting to unix socket");
    exit(2);
  }

  OldstyleServerOffer offer;

  int bytes_read = recv(socket_fd, &offer, sizeof(offer), 0);
  offer.hostify();

  if (bytes_read != 152) {
    fprintf(stderr, "Error: %d bytes read instead of valid 152 bytes", bytes_read);
    exit(3);
  }

  /* fmt::print("{:-^30}\n", name); */
  print_header(name);
  print_server_offer(offer);

  size = offer.export_size;
}

NbdConnection::~NbdConnection() { close(socket_fd); }

