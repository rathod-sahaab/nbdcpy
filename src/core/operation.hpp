#ifndef OPERATION_HPP
#define OPERATION_HPP
#include <sys/types.h>

enum class OperationState : u_int32_t {
  EMPTY,
  REQUESTING,
  WAITING,
  READING,
  WRITING,
  CONFIRMING,
};

/**
 * May be add a buffer attribute, to avoid reallocating buffers
 * size = max_size = MAX_PACKET_SIZE + sizeof(RequestHeader)
 * as sizeof(RequestHeader) > sizeof(SimpleReplyHeader)
 */
struct Operation {
  u_int64_t handle;
  u_int64_t offset;
  u_int32_t length;
  OperationState state;
  char *buffer;

public: // functions
  /// a utility function to get the state char for debugging
  char getStateChar() const noexcept;
} __attribute__((__packed__));

#endif // OPERATION_HPP
