#include "uring_ops.hpp"
#include "nbd_types.hpp"

int submit_read_request(NbdConnection &conn, io_uring *ring_ptr, u_int64_t p_handle,
                        u_int64_t p_offset, u_int32_t p_length) {

  const RequestHeader rqh =
      RequestHeader::create_network_byteordered(0, 0, p_handle, p_offset, p_length);

  io_uring_sqe *sqe = io_uring_get_sqe(ring_ptr);

  io_uring_prep_send(sqe, conn.get_socket(), &rqh, sizeof(rqh), 0);
  io_uring_submit(ring_ptr);

  return 0;
}

void submit_read() {

}
