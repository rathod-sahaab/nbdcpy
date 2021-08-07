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
constexpr const int MAX_INFLIGHT_REQUESTS = 16;

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

  // job description
  const off_t total_size = nbd_src.get_size();
  off_t offset = 0, bytes_left = total_size;

  // state params
  int queued_requests = 0, inqueue_read_header_reqs = 0;

  /*
   * This denotes how many read requests were fired - Reply reads added to
   * socket before the program ends all read requests must have corresponding
   * read header i.e. surplus should be 0
   */
  int read_request_surplus = 0;

  std::vector<Operation> operations(MAX_INFLIGHT_REQUESTS);

  // fill the operations vector first
  for (int i = 0; i < MAX_INFLIGHT_REQUESTS and offset < total_size; ++i) {
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

  // submit the enqueued operations above to io_ring
  io_uring_submit(&ring);

  while (bytes_left > 0 or queued_requests > 0) {

    std::cout << "Operation States:\n";
    for (const auto &op : operations) {
      std::cout << getStateChar(op.state) << " ";
    }
    std::cout << "\n";
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

    struct io_uring_cqe *cqe = nullptr; // mutated by io_uring_wait_cqe
    /*
     * We must wait for a completion as there are already MAX_INFLIGHT_REQUESTS
     * in the io_uring.
     */
    int ret = io_uring_wait_cqe(&ring, &cqe);

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

    // we insert Operation* or nullptr hence casting is safe
    void *vp = io_uring_cqe_get_data(cqe);
    UringUserData *const uring_user_data = (UringUserData *)vp;

    if (not uring_user_data) {
      std::cout << "uring_user_data null" << std::endl;
    }
    if (uring_user_data->is_read) {
      std::cout << "CQE uring user data is read" << std::endl;
      // read operation, data field points to buffer where SimpleReplyHeader is
      // read
      SimpleReplyHeader *const srh = (SimpleReplyHeader *)uring_user_data->data;
      srh->hostify();

      fmt::print("Reply handle: {} ({})\n", (unsigned long)srh->handle,
                 (unsigned long)be64toh(srh->handle));

      Operation &op = operations[(unsigned long)srh->handle];

      if (op.state == OperationState::WAITING) {
        // if cqe was successfull for operation[handle] then, we MUST
        // have issued read header command in that case WAITING -> READING
        // should have been done, but we can't know which handle will we get
        // unless we read the header so we are doing it now
        op.state = OperationState::READING;
      }

      // TODO: check for nbd errors
      if (op.state == OperationState::READING) {
        /* std::cout << "Reading the rest of the data for socket" << std::endl;
         */

        // FIXME: This MAY NOT move forward the io_uring's offset, so read may
        // be buggy
        const auto ret = recv(nbd_src.get_socket(),
                              op.buffer + sizeof(RequestHeader), op.length, 0);
        if (ret < 0) {
          fmt::print("recv error\n");
          exit(1);
        }
        if (ret < op.length) {
          fmt::print("Error: less than operation_ref.length bytes read{}\n",
                     ret);
          exit(1);
        }

        enqueue_write(nbd_dest, &ring, op.handle, op.offset, op.length,
                      op.buffer);

        op.state = OperationState::WRITING;

        /**
         * we moved to next step of operation i.e. writing
         * so currently there is no read request in the
         * works and hence we add another request now.
         */
        if (read_request_surplus > 0) {
          enqueue_read_header(nbd_src, &ring);
          read_request_surplus--;
        } else {
          inqueue_read_header_reqs = 0;
        }

        io_uring_submit(&ring);
      } else {
        // can only be confirming, start another request at new offset if all of
        // the file is read set empty
        if (bytes_left > 0) {
          // start a new request
          const auto length = std::min(MAX_PACKET_SIZE, bytes_left);
          op.length = length;
          op.offset = offset;

          enqueue_send_read_request(nbd_src, &ring, op.handle, op.offset, op.length);
          op.state = OperationState::REQUESTING;

          offset += length;
          bytes_left -= length;
          io_uring_submit(&ring);
        } else {
          queued_requests--;
          op.state = OperationState::EMPTY;
          io_uring_cqe_seen(&ring, cqe);
          continue;
        }
      }

      delete srh;
    } else {
      // write operation data points to request which this belongs to

      RequestHeader *const request_ptr = (RequestHeader *)uring_user_data->data;
      request_ptr->hostify();

      // every field was created in network byte order.
      Operation &operation_ref = operations[request_ptr->handle];
      // operation state is actually the previous state so we need to advance it
      if (operation_ref.state == OperationState::REQUESTING) {
        /* std::cout << "Transitioning from REQUESTING to READING" << std::endl;
         */
        // NBD read request was successfull
        read_request_surplus++;

        if (inqueue_read_header_reqs == 0) {
          // if there is no read header request inflight
          inqueue_read_header_reqs = 1;
          enqueue_read_header(nbd_src, &ring);
          io_uring_submit(&ring);
        }
        operation_ref.state = OperationState::WAITING;

        std::free(request_ptr);
      } else {
        // writing
        /* std::cout << "Transitioning from WRITING to CONFIRMING" << std::endl;
         */
        enqueue_read_header(nbd_dest, &ring);
        operation_ref.state = OperationState::CONFIRMING;
        io_uring_submit(&ring);
      }
    }
    delete uring_user_data;
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
