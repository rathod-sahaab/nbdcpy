#include "uring_ops.hpp"
#include "nbd_types.hpp"
#include "uring_user_data.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <liburing.h>

void enqueue_read_request(NbdConnection &conn, io_uring *ring_ptr,
                          u_int64_t p_handle, u_int64_t p_offset,
                          u_int32_t p_length, Operation *operation_ptr) {

  // Can't allocate this on the stack because io_uring might process is after
  // the function's lifetime
  // TODO: delete/free this memory
  RequestHeader *const rqh = RequestHeader::allocate_network_byteordered(
      0, NBD_CMD_READ, p_handle, p_offset, p_length);

  io_uring_sqe *sqe = io_uring_get_sqe(ring_ptr);

  // Added rqh to delete on completion
  UringUserData *uring_user_data = new UringUserData(rqh, false);

  io_uring_sqe_set_data(sqe, uring_user_data);
  // to determine which nbd request does the entry belongs to
  // will persist to cqe.

  // queue the write operation to socket
  io_uring_prep_send(sqe, conn.get_socket(), rqh, sizeof(SimpleReplyHeader), 0);
}
void submit_read_request(NbdConnection &conn, io_uring *ring_ptr,
                         u_int64_t p_handle, u_int64_t p_offset,
                         u_int32_t p_length, Operation *operation_ptr) {
  enqueue_read_request(conn, ring_ptr, p_handle, p_offset, p_length,
                       operation_ptr);
  io_uring_submit(ring_ptr);
}

void enqueue_read_header(NbdConnection &conn, io_uring *ring_ptr) {
  SimpleReplyHeader *srh_ptr = new SimpleReplyHeader();
}
