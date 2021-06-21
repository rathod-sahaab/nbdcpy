#ifndef NBD_TYPES_HPP
#define NBD_TYPES_HPP
/**
 * This file contains various nbd types and constants which are used to submit requests
 * and read responses from the nbd server. The types are customised for usage by client
 * additional properties and functions should be added for use by the server.
 */

#include <endian.h>
#include <netinet/in.h>
#include <sys/types.h>

/**
 * NBD request magic in host's format/byte-order
 */
constexpr const u_int32_t NBD_REQUEST_MAGIC_HOST = 0x25609513;
/**
 * NBD simple reply magic in host's format/byte-order
 */
constexpr const u_int32_t NBD_SIMPLE_REPLY_MAGIC_HOST = 0x67446698;
/**
 * NBD structured reply magic in host's format/byte-order
 */
constexpr const u_int32_t NBD_STRUCTURED_REPLY_MAGIC_HOST = 0x668e33ef;

struct RequestHeader {
  const u_int32_t magic = htonl(NBD_REQUEST_MAGIC_HOST); // NBD_REQUEST_MAGIC
  u_int16_t command_flags;
  u_int16_t type;
  /**
   * Handle is unique identifier of a request, which is sent back by the server
   * along reply for the request so we can determine to which request does the
   * response belong.
   */
  u_int64_t handle;
  /**
   * The lseek position from which we want to start reading data.
   */
  u_int64_t offset;
  /**
   * Number of bytes that we want to read
   */
  u_int32_t length;

  /*
   * You cant create default object, as it's sent over network, byteorder has to be
   * modified.
   */
  RequestHeader() = delete;

  /**
   * Use this to create RequestHeader in network byteorder.
   */
  static RequestHeader create_network_byteordered(u_int16_t p_command, u_int16_t p_type,
                                                  u_int64_t p_handle, u_int64_t p_offset,
                                                  u_int32_t p_length) {
    // As this data-structure is meant to be sent over the nework we only allow it to be
    // created by this factory function. we can do this in constructor but it would have
    // hidden the fact that variables are in network byte-order and hence are tainted.

    p_command = htons(p_command);
    p_type = htons(p_type);
    p_handle = htobe64(p_handle);
    p_offset = htobe64(p_offset);
    p_length = htobe32(p_length);
    return RequestHeader(p_command, p_type, p_handle, p_offset, p_length);
  }

private:
  /**
   * Create RequestHeader object, we don't want people to use it because for sending
   * it should be in network byte-order. Hence we have a factory function that creates it
   * for us.
   */
  RequestHeader(u_int16_t p_command, u_int16_t p_type, u_int64_t p_handle,
                u_int64_t p_offset, u_int32_t p_length)
      : command_flags{p_command},
        type{p_type},
        handle{p_handle},
        offset{p_offset},
        length{p_length} {}

} __attribute__((packed));

/**
 * Header to extract when we receive a reply from the server
 * This is meant to be receive only and not send hence lacks
 * encoding to network byte-order and only provides decoding
 * from network byte-order.
 */
struct SimpleReplyHeader {
  u_int32_t simple_reply_magic;
  u_int32_t errors;
  u_int64_t handle;

  /**
   * When read from network, byteorder is in big-endian format so we fix the order to get
   * correct values.
   */
  void hostify() {
    if (is_hostified()) {
      return;
    }
    simple_reply_magic = ntohl(simple_reply_magic);
    errors = ntohl(simple_reply_magic);
    handle = be64toh(handle);
  }

  /**
   * Returns if we already ran hostify we cant store the information in a boolean as it
   * would have be a member variable and will cause faulty reads.
   * Hence, we check if magic is in host format as if we already hostified it, we made it
   * in host's byte order.
   */
  bool is_hostified() { return simple_reply_magic == NBD_SIMPLE_REPLY_MAGIC_HOST; }
};

/**
 * Header to extract when we receive a reply from the server
 * This is meant to be receive only and not send hence lacks
 * encoding to network byte-order and only provides decoding
 * from network byte-order.
 */
struct StructuredReplyHeader {
  u_int32_t magic;
  u_int16_t flags;
  u_int16_t type;
  u_int64_t handle;
  u_int32_t length;

  /**
   * When read from network, byteorder is in big-endian format so we fix the order to get
   * correct values.
   */
  void hostify() {
    if (is_hostified()) {
      // already hostified, running again will de-hostify
      return;
    }
    magic = ntohl(magic);
    flags = ntohs(flags);
    type = ntohs(type);
    handle = be64toh(handle);
    length = htonl(length);
  }

  /**
   * Returns if we already ran hostify we cant store the information in a boolean as it
   * would have be a member variable and will cause faulty reads.
   * Hence, we check if magic is in host format as if we already hotified it, we made it
   * in host's byte order.
   */
  bool is_hostified() { return magic == NBD_STRUCTURED_REPLY_MAGIC_HOST; }
} __attribute__((packed));

#endif // NBD_TYPES_HPP
