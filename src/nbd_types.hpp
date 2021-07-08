#ifndef NBD_TYPES_HPP
#define NBD_TYPES_HPP
/**
 * This file contains various nbd types and constants which are used to submit
 * requests and read responses from the nbd server. The types are customised for
 * usage by client additional properties and functions should be added for use
 * by the server.
 */

#include <endian.h>
#include <netinet/in.h>
#include <sys/types.h>

// clang-format off
#define NBD_FLAG_HAS_FLAGS         (1 << 0)
#define NBD_FLAG_READ_ONLY         (1 << 1)
#define NBD_FLAG_SEND_FLUSH        (1 << 2)
#define NBD_FLAG_SEND_FUA          (1 << 3)
#define NBD_FLAG_ROTATIONAL        (1 << 4)
#define NBD_FLAG_SEND_TRIM         (1 << 5)
#define NBD_FLAG_SEND_WRITE_ZEROES (1 << 6)
#define NBD_FLAG_SEND_DF           (1 << 7)
#define NBD_FLAG_CAN_MULTI_CONN    (1 << 8)
#define NBD_FLAG_SEND_CACHE        (1 << 10)
#define NBD_FLAG_SEND_FAST_ZERO    (1 << 11)

/* NBD commands. */
#define NBD_CMD_READ              0
#define NBD_CMD_WRITE             1
#define NBD_CMD_DISC              2 /* Disconnect. */
#define NBD_CMD_FLUSH             3
#define NBD_CMD_TRIM              4
#define NBD_CMD_CACHE             5
#define NBD_CMD_WRITE_ZEROES      6
#define NBD_CMD_BLOCK_STATUS      7

#define NBD_CMD_FLAG_FUA       (1<<0)
#define NBD_CMD_FLAG_NO_HOLE   (1<<1)
#define NBD_CMD_FLAG_DF        (1<<2)
#define NBD_CMD_FLAG_REQ_ONE   (1<<3)
#define NBD_CMD_FLAG_FAST_ZERO (1<<4)

/* NBD error codes. */
#define NBD_SUCCESS     0
#define NBD_EPERM       1
#define NBD_EIO         5
#define NBD_ENOMEM     12
#define NBD_EINVAL     22
#define NBD_ENOSPC     28
#define NBD_EOVERFLOW  75
#define NBD_ENOTSUP    95
#define NBD_ESHUTDOWN 108
// clang-format on

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
  u_int32_t magic; // NBD_REQUEST_MAGIC
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
   * You cant create default object, as it's sent over network, byteorder has to
   * be modified.
   */
  RequestHeader() = delete;

  RequestHeader(u_int16_t p_command, u_int16_t p_type, u_int64_t p_handle,
                u_int64_t p_offset, u_int32_t p_length)
      : magic{NBD_REQUEST_MAGIC_HOST}, command_flags{p_command}, type{p_type},
        handle{p_handle}, offset{p_offset}, length{p_length} {}

  /**
   * Just like the construct, created for using with malloc where you can't new
   */
  void intialize(u_int16_t p_command, u_int16_t p_type, u_int64_t p_handle,
                 u_int64_t p_offset, u_int32_t p_length) {

    magic = NBD_REQUEST_MAGIC_HOST;
    command_flags = p_command;
    type = p_type;
    handle = p_handle;
    offset = p_offset;
    length = p_length;
  }
  /**
   * Use this to encode RequestHeader in network byteorder.
   */
  void networkify() {
    if (is_networkified()) {
      return;
    }
    magic = htobe32(magic);
    command_flags = htobe16(command_flags);
    type = htobe16(type);
    handle = htobe64(handle);
    offset = htobe64(offset);
    length = htobe32(length);
  }

  /**
   * Convert request encoded in network byteorder to host
   */
  void hostify() {
    if (not is_networkified()) {
      return;
    }
    magic = be32toh(magic);
    command_flags = be16toh(command_flags);
    type = be16toh(type);
    handle = be64toh(handle);
    offset = be64toh(offset);
    length = be32toh(length);
  }

  bool is_networkified() { return magic == htobe32(NBD_REQUEST_MAGIC_HOST); }
} __attribute__((__packed__));

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
   * When read from network, byteorder is in big-endian format so we fix the
   * order to get correct values.
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
   * Returns if we already ran hostify we cant store the information in a
   * boolean as it would have be a member variable and will cause faulty reads.
   * Hence, we check if magic is in host format as if we already hostified it,
   * we made it in host's byte order.
   */
  bool is_hostified() {
    return simple_reply_magic == NBD_SIMPLE_REPLY_MAGIC_HOST;
  }
} __attribute__((__packed__));

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
   * When read from network, byteorder is in big-endian format so we fix the
   * order to get correct values.
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
   * Returns if we already ran hostify we cant store the information in a
   * boolean as it would have be a member variable and will cause faulty reads.
   * Hence, we check if magic is in host format as if we already hotified it, we
   * made it in host's byte order.
   */
  bool is_hostified() { return magic == NBD_STRUCTURED_REPLY_MAGIC_HOST; }
} __attribute__((__packed__));

#endif // NBD_TYPES_HPP
