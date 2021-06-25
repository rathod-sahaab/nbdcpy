#include "uring_ops.hpp"
#include "nbd_types.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <liburing.h>

void enqueue_read_request(NbdConnection &conn, io_uring *ring_ptr,
                          u_int64_t p_handle, u_int64_t p_offset,
                          u_int32_t p_length, Operation *operation_ptr) {

  const RequestHeader rqh = RequestHeader::create_network_byteordered(
      0, 0, p_handle, p_offset, p_length);

  io_uring_sqe *sqe = io_uring_get_sqe(ring_ptr);

  io_uring_sqe_set_data(sqe, operation_ptr);
  // to determine which nbd request does the entry belongs to
  // will persist to cqe.

  // queue the write operation to socket
  io_uring_prep_send(sqe, conn.get_socket(), &rqh, sizeof(rqh), 0);
}
void submit_read_request(NbdConnection &conn, io_uring *ring_ptr,
                         u_int64_t p_handle, u_int64_t p_offset,
                         u_int32_t p_length, Operation *operation_ptr) {
  enqueue_read_request(conn, ring_ptr, p_handle, p_offset, p_length,
                       operation_ptr);
  io_uring_submit(ring_ptr);
}

void submit_read() {}
