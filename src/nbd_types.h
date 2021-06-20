#ifndef NBD_TYPES_HPP
#define NBD_TYPES_HPP

#include <sys/types.h>
struct ClientRequest {
  const u_int32_t magic = 0x25609513; // NBD_REQUEST_MAGIC
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
};
#endif // NBD_TYPES_HPP
