#ifndef URING_OPS_HPP
#define URING_OPS_HPP

#include "nbd_connection.hpp"
#include "operation.hpp"
#include <liburing.h>
#include <sys/types.h>

/**
 * Inserts nbd read request in the submission queue of io_uring
 * NOTE: does not call io_uring_submit.
 */
void enqueue_read_request(NbdConnection &conn, io_uring *ring_ptr,
                        u_int64_t p_handle, u_int64_t p_offset,
                        u_int32_t p_length, Operation *operation_ptr);

/**
 * Same as enquqe_read_request but also call io_uring_submit.
 * To be used when we are sure this will be called independently from other
 * io_uring submissions.
 */
void submit_read_request(NbdConnection &conn, io_uring *ring_ptr,
                        u_int64_t p_handle, u_int64_t p_offset,
                        u_int32_t p_length, Operation *operation_ptr);

#endif // URING_OPS_HPP
