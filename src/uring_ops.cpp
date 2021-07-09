#include "uring_ops.hpp"
#include "fmt/core.h"
#include "nbd_types.hpp"
#include "uring_user_data.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <ostream>

constexpr const bool IS_READ = true, IS_WRITE = true;

void enqueue_send_read_request(NbdConnection &conn, io_uring *ring_ptr,
                               u_int64_t p_handle, u_int64_t p_offset,
                               u_int32_t p_length) {
  fmt::print("Sending read request to '{}' for {} bytes at offset {} and "
             "handle [{}]\n",
             conn.get_name(), p_length, p_offset, p_handle);

  // Can't allocate this on the stack because io_uring might process it
  // after the function's lifetime
  RequestHeader *const rqh =
      (RequestHeader *)std::malloc(sizeof(RequestHeader));
  rqh->intialize(0, NBD_CMD_READ, p_handle, p_offset, p_length);
  rqh->networkify();

  io_uring_sqe *sqe = io_uring_get_sqe(ring_ptr);

  // Added rqh to delete on completion
  UringUserData *const uring_user_data =
      (UringUserData *)std::malloc(sizeof(UringUserData));
  uring_user_data->initiaize(rqh, IS_WRITE);
  // IS_WRITE because we send/write the socket

  if (not uring_user_data) {
    std::cout << "uring_user_data null at insertion" << std::endl;
  }
  fmt::print("Inserted Uring userdata: <{:#x}>\n",
             (unsigned long)uring_user_data);

  // queue the write operation to socket
  io_uring_prep_send(sqe, conn.get_socket(), rqh, sizeof(*rqh), 0);
  // set_data should be called after send/write as prep_send
  io_uring_sqe_set_data(sqe, uring_user_data);
  // to determine which nbd request does the entry belongs to
  // will persist to cqe.
}
void submit_read_request(NbdConnection &conn, io_uring *ring_ptr,
                         u_int64_t p_handle, u_int64_t p_offset,
                         u_int32_t p_length) {
  enqueue_send_read_request(conn, ring_ptr, p_handle, p_offset, p_length);
  io_uring_submit(ring_ptr);
}

void enqueue_read_header(NbdConnection &conn, io_uring *ring_ptr) {
  fmt::print("Reading headers from socket of {}\n", conn.get_name());
  SimpleReplyHeader *srh_ptr = new SimpleReplyHeader();
  io_uring_sqe *sqe = io_uring_get_sqe(ring_ptr);
  io_uring_prep_read(sqe, conn.get_socket(), srh_ptr, sizeof(SimpleReplyHeader),
                     0);

  UringUserData *uring_user_data = new UringUserData(srh_ptr, IS_READ);
  io_uring_sqe_set_data(sqe, uring_user_data);
}

void enqueue_write(const NbdConnection &conn, io_uring *ring_ptr,
                   u_int64_t p_handle, u_int64_t p_offset, u_int32_t p_length,
                   void *buffer) {
  // prepare buffer will only overwrite the first 28 bytes
  fmt::print("Writing data to '{}' for {} bytes at offset {} and "
             "handle [{}]\n",
             conn.get_name(), p_length, p_offset, p_handle);
  RequestHeader *const rqh = (RequestHeader *)buffer;

  rqh->intialize(0, NBD_CMD_WRITE, p_handle, p_offset, p_length);
  rqh->networkify();

  io_uring_sqe *sqe = io_uring_get_sqe(ring_ptr);

  io_uring_prep_send(sqe, conn.get_socket(), buffer,
                     sizeof(RequestHeader) + p_length, 0);

  UringUserData *uring_user_data = new UringUserData(buffer, IS_WRITE);
  io_uring_sqe_set_data(sqe, uring_user_data);
}
