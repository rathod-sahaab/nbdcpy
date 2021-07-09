#include "cli-params.hpp"
#include "fmt/core.h"
#include "nbd_connection.hpp"
#include "nbd_types.hpp"
#include "oldstyle.hpp"
#include "operation.hpp"
#include "uring_ops.hpp"
#include "uring_user_data.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <endian.h>
#include <liburing.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
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
  for (int i = 0; i < MAX_INFLIGHT_REQUESTS and offset < total_size; ++i) {
    fmt::print("offest: {}, bytes_left: {}\n", offset, bytes_left);
    auto &operation = operations[i];

    operation.handle = i;
    operation.state = OperationState::REQUESTING;
    operation.offset = offset;
    operation.length = std::min(MAX_PACKET_SIZE, bytes_left);

    // we read Replies from source in this buffer
    // and use this buffer to send data to destination
    // our max data size can be MAX_PACKET_SIZE add to that the RequestHeader
    // READ reply from source: 	     HEADER___16bytes............DATA________
    // WRITE request to destination: HEADER___28bytes____________DATA________
    // '.' represents useless bytes, other chars useful bytes
    // Since SimpleReplyHeader is only 16 bytes while RequestHeader is 28
    // RequestHeader allocating for RequestHeader is suffice.
    // CAUTION: You MUST always offset 28 bytes for data in any case
    // void* data = buffer + 28 i.e. buffer + sizeof(RequestHeader)
    operation.buffer = (char *)malloc(sizeof(RequestHeader) + MAX_PACKET_SIZE);

    enqueue_send_read_request(nbd_src, &ring, i, operation.offset,
                              operation.length);

    queued_requests++;
    offset += operation.length;
    bytes_left -= operation.length;
  }

  fmt::print("Out of for loop\n");

  // submit the enqueued operations above to io_ring
  io_uring_submit(&ring);

  while (bytes_left > 0 or queued_requests > 0) {
    /*
     * here 'operation' is a complete copy operation
     *
     * This loop waits for the operations completion and processes their
     * various stages. After one operation is complete is starts another
     * operation reusing complete process's slot in operations vector.
     *
     * But, after this loop there will be operations that are pending i.e. are
     * in REQUESTING, READING, WRITING, CONFIRMING state. But they will be the
     * final operations i.e. there will be no more work after they are done
     * being processed. Hence, one more loop will be required to process those
     * half completed operation.
     */

    fmt::print("before wait\n");

    struct io_uring_cqe *cqe = nullptr; // mutated by io_uring_wait_cqe
    /*
     * We must wait for a completion as there are already MAX_INFLIGHT_REQUESTS
     * in the io_uring.
     */
    int ret = io_uring_wait_cqe(&ring, &cqe);

    fmt::print("after wait\n");

    if (ret != 0) {
      fmt::print("io_uring_cqe ret");
      exit(1);
    }

    if (not cqe) {
      std::cout << "cqe is null" << std::endl;
    }

    if (cqe->res < 0) {
      std::cout << "cqe res: Async operation failed" << std::endl;
    }

    std::cout << " Before uring_user_data retrival" << std::endl;
    // we insert Operation* or nullptr hence casting is safe
    void *vp = io_uring_cqe_get_data(cqe);
    fmt::print("get data vp: <{:#x}>\n", (unsigned long)vp);
    UringUserData *const uring_user_data = (UringUserData *)vp;
    fmt::print("Inserted Uring userdata: <{:#x}>\n",
               (unsigned long)uring_user_data);

    if (not uring_user_data) {
      std::cout << "uring_user_data null" << std::endl;
    }
    std::cout << " After retrival" << std::endl;

    if (uring_user_data->is_read) {
      // read operation, data field points to buffer where SimpleReplyHeader is
      // read
      SimpleReplyHeader *const srh = (SimpleReplyHeader *)uring_user_data->data;
      srh->hostify();

      fmt::print("Reply handle: {} ({})\n", (unsigned long)srh->handle,
                 (unsigned long)be64toh(srh->handle));

      Operation &operation_ref = operations[(unsigned long)srh->handle];

      // TODO: check for errors
      if (operation_ref.state == OperationState::READING) {
        const auto ret = recv(nbd_src.get_socket(),
                              operation_ref.buffer + sizeof(RequestHeader),
                              operation_ref.length, 0);
        if (ret < 0) {
          fmt::print("recv error\n");
          exit(1);
        }
        if (ret < operation_ref.length) {
          fmt::print("Error: less than operation_ref.length bytes read\n");
          exit(1);
        }

        enqueue_write(nbd_dest, &ring, operation_ref.handle,
                      operation_ref.offset, operation_ref.length,
                      operation_ref.buffer);

        operation_ref.state = OperationState::WRITING;
        io_uring_submit(&ring);

      } else {
        // can only be confirming, start another request at new offset if all of
        // the file is read set empty
        if (bytes_left > 0) {
          // start a new request
          const auto length = std::min(MAX_PACKET_SIZE, bytes_left);

          enqueue_send_read_request(nbd_src, &ring, operation_ref.handle,
                                    offset, length);
          operation_ref.state = OperationState::REQUESTING;

          offset += length;
          bytes_left -= length;
          io_uring_submit(&ring);
        } else {
          queued_requests--;
          operation_ref.state = OperationState::EMPTY;
          continue;
        }
      }

      delete srh;
    } else {
      // write operation data points to request which this belongs to

      RequestHeader *const request_ptr = (RequestHeader *)uring_user_data->data;
      request_ptr->hostify();

      fmt::print("Request Header: {} ({})\n",
                 (unsigned long)request_ptr->handle,
                 (unsigned long)be64toh(request_ptr->handle));

      // every field was created in network byte order.
      Operation &operation_ref = operations[request_ptr->handle];
      // operation state is actually the previous state so we need to advance it
      if (operation_ref.state == OperationState::REQUESTING) {
        enqueue_read_header(nbd_src, &ring);
        operation_ref.state = OperationState::READING;
        io_uring_submit(&ring);

        // only delete here because writing used operation_ref.buffer
        std::free(request_ptr);
      } else {
        // writing
        enqueue_read_header(nbd_dest, &ring);
        operation_ref.state = OperationState::CONFIRMING;
        io_uring_submit(&ring);
      }
    }
    fmt::print("uring checked");

    free(uring_user_data);
    io_uring_cqe_seen(&ring, cqe);
  }

  for (const auto &operation : operations) {
    // Were exiting so it's not necessary to free the buffers but free every
    // malloc anyways
    free(operation.buffer);
  }

  io_uring_queue_exit(&ring);
  return 0;
}
