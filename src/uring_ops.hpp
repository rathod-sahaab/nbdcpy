#ifndef URING_OPS_HPP
#define URING_OPS_HPP

#include "nbd_connection.hpp"
#include <liburing.h>
#include <sys/types.h>

int submit_read_request(NbdConnection &conn, io_uring *ring_ptr, u_int64_t p_handle,
                        u_int64_t p_offset, u_int32_t p_length);

#endif // URING_OPS_HPP
