#include "cli-params.hpp"
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
  std::cout << cli_params.src_port << " --> " << cli_params.dest_port << "\n";

  NbdConnection nbd_src(cli_params.src_port, &ring);
  NbdConnection nbd_dest(cli_params.dest_port, &ring);

  if (nbd_dest.get_size() < nbd_src.get_size()) {
    std::cout << "Source is larger than destination, will cause data loss"
              << "\n";
    exit(1);
  }
  const off_t total_size = nbd_src.get_size();
  off_t offset = 0, bytes_left = total_size;

  return 0;
}
