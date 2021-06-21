#include "nbd_connection.hpp"
#include "nbd_types.hpp"
#include "oldstyle.hpp"
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

NbdConnection::NbdConnection(const int port, io_uring *ring) : ring_ptr{ring} {
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

  print_server_offer(offer);

  size = offer.export_size;
}

NbdConnection::~NbdConnection() { close(socket_fd); }

int NbdConnection::submit_read_request(u_int64_t p_handle, u_int64_t p_offset,
                                       u_int32_t p_length) {
  const RequestHeader rqh =
      RequestHeader::create_network_byteordered(0, 0, p_handle, p_offset, p_length);

  io_uring_sqe *sqe = io_uring_get_sqe(ring_ptr);

  io_uring_prep_send(sqe, socket_fd, &rqh, sizeof(rqh), 0);
  io_uring_submit(ring_ptr);

  return 0;
}
