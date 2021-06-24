#include "cli-params.hpp"
#include "fmt/core.h"
#include "nbd_connection.hpp"
#include "nbd_types.hpp"
#include "oldstyle.hpp"
#include <cstdlib>
#include <liburing.h>

constexpr const int MAX_PACKET_SIZE = 512; // bytes
constexpr const int MAX_INFLIGHT_REQUESTS = 6;

int main(int argc, char **argv) {
  struct io_uring ring;
  io_uring_queue_init(MAX_INFLIGHT_REQUESTS, &ring, 0);

  const auto cli_params = parse_cli(argc, argv);
  fmt::print("Copying from port {} to port {}.\n", cli_params.src_port,
             cli_params.dest_port);

  NbdConnection nbd_src("source", cli_params.src_port, &ring);
  NbdConnection nbd_dest("destination", cli_params.dest_port, &ring);

  if (nbd_dest.get_size() < nbd_src.get_size()) {
    fmt::print("Source is larger than destination, will cause data loss\n");
    io_uring_queue_exit(&ring);
    exit(1);
  }
  const off_t total_size = nbd_src.get_size();
  off_t offset = 0, bytes_left = total_size;
  int queued_requests = 0;

  io_uring_queue_exit(&ring);
  return 0;
}
