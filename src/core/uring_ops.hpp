#ifndef URING_OPS_HPP
#define URING_OPS_HPP

#include "../proto/nbd_connection.hpp"
#include "operation.hpp"
#include <liburing.h>
#include <sys/types.h>

/**
 * Inserts nbd read request in the submission queue of io_uring
 * NOTE: does not call io_uring_submit.
 */
void enqueue_send_read_request(NbdConnection &conn, io_uring *ring_ptr,
                               u_int64_t p_handle, u_int64_t p_offset,
                               u_int32_t p_length);

/**
 * Same as enquqe_read_request but also call io_uring_submit.
 * To be used when we are sure this will be called independently from other
 * io_uring submissions.
 */
void submit_read_request(NbdConnection &conn, io_uring *ring_ptr,
                         u_int64_t p_handle, u_int64_t p_offset,
                         u_int32_t p_length);

void enqueue_read_header(NbdConnection &conn, io_uring *ring_ptr);

void enqueue_write(const NbdConnection &conn, io_uring *ring_ptr,
                   u_int64_t p_handle, u_int64_t p_offset, u_int32_t p_length,
                   void *buffer);

#endif // URING_OPS_HPP
