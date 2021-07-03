#ifndef IOURING_USER_DATA_HPP
#define IOURING_USER_DATA_HPP

/**
 * A structure to store data about our io_uring request
 * In case submiting write request to io_uring completion happens as soon as the
 * write is performed and hence cqe is always for the write request we submited
 * to io_uring.
 *
 * But, in case of reading sockets, io_uring can't reliabily determine for which
 * request do we have data so here cqe just denotes a read is available. Now
 * it's our responsibility to determine the read is for which request,
 * fortunately NBD protocol has handle field to help us determine that.
 *
 * In case of write operation, due to reliable mapping of CQE we can just attach
 * RequestHeader to it denoting the request which this CQE belongs to and also
 * so that we can free that buffer.
 *
 * But, in case of reading we have to submit request to read SimpleReplyHeader.
 * Now we also need to attach the buffer we read the SimpleReplyHeader into to
 * cqe user_data field.
 *
 * Thus this indirection was necessary, one field to determine if cqe was for a
 * socket read, if it was a read data would point to the buffer in which
 * SimpleReplyHeader was read in. if it was for a write it will point to
 * RequestHeader because we need to free that buffer.
 */
struct UringUserData {
  // operation in case of write/send operations, reply buffer for reads
  void *data;
  bool is_read;

  UringUserData(void *p_data, bool p_is_read)
      : data{p_data}, is_read{p_is_read} {}
};

#endif // IOURING_USER_DATA_HPP
