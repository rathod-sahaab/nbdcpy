#include "cli-params.hpp"
#include "fmt/core.h"
#include "nbd_connection.hpp"
#include "nbd_types.hpp"
#include "oldstyle.hpp"
#include "operation.hpp"
#include "uring_ops.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <liburing.h>
#include <vector>

constexpr const off_t MAX_PACKET_SIZE = 512; // bytes
constexpr const int MAX_INFLIGHT_REQUESTS = 6;

int main(int argc, char **argv) {
  struct io_uring ring;
  io_uring_queue_init(MAX_INFLIGHT_REQUESTS, &ring, 0);

  const auto cli_params = parse_cli(argc, argv);
  fmt::print("Copying from port {} to port {}.\n", cli_params.src_port,
             cli_params.dest_port);

  NbdConnection nbd_src("source", cli_params.src_port);
  NbdConnection nbd_dest("destination", cli_params.dest_port);

  if (nbd_dest.get_size() < nbd_src.get_size()) {
    fmt::print("Source is larger than destination, will cause data loss\n");
    io_uring_queue_exit(&ring);
    exit(1);
  }
  const off_t total_size = nbd_src.get_size();
  off_t offset = 0, bytes_left = total_size;
  int queued_requests = 0;

  std::vector<Operation> operations(MAX_INFLIGHT_REQUESTS);

  // fill the operations vector first
  for (int i = 0; i < MAX_INFLIGHT_REQUESTS; ++i) {
    auto &operation = operations[i];

    operation.state = OperationState::REQUESTING;
    operation.offset = offset;
    operation.length = std::min(MAX_PACKET_SIZE, bytes_left);

    enqueue_read_request(nbd_src, &ring, i, operation.offset, operation.length,
                         &operations[i]);

    offset += MAX_PACKET_SIZE;
    bytes_left -= MAX_PACKET_SIZE;
  }

  // submit the enqueued operations above to io_ring
  io_uring_submit(&ring);

  while (bytes_left > 0) {
    // must wait for a completion as there are already MAX_INFLIGHT_REQUESTS in
    // the io_uring
    struct io_uring_cqe *cqe = nullptr;
    // cqe pointer's value is changed by wait_cqe

    int ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret != 0) {
      perror("io_uring_cqe ret");
      exit(1);
    }
    if (!cqe) {
      perror("io_uring_cqe cqe");
      exit(1);
    }

    // we insert operation_ptr so we are sure and hence casting is safe
    Operation *operation_ptr = (Operation *)io_uring_cqe_get_data(cqe);

    // operation state is actually the previous state so we need to advance it
    // now
    switch (operation_ptr->state) {
    case OperationState::REQUESTING:
      // TODO: submit socket.recv to read from source.
      operation_ptr->state = OperationState::READING;
      break;
    case OperationState::READING:
      // TODO: enqueue socket.send to send data to destination
      break;
    case OperationState::WRITING:
      // TODO: enqueue socket.recv to read confirmation from destination
      break;
    case OperationState::CONFIRMING:
      // TODO: enqueue new read request for another offset (next request).
      break;
    case OperationState::EMPTY:
      // This should not be encountered if there are still request
      break;
    }
  }

  io_uring_queue_exit(&ring);
  return 0;
}
