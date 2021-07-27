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

char getStateChar(OperationState state) {
  switch (state) {
  case OperationState::REQUESTING:
    return 'r';
  case OperationState::WAITING:
    return 'w';
  case OperationState::READING:
    return 'R';
  case OperationState::WRITING:
    return 'W';
  case OperationState::CONFIRMING:
    return 'C';
  case OperationState::EMPTY:
    return 'E';
  }
}

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
  char *buffer; // not sure
} __attribute__((__packed__));

#endif // OPERATION_HPP
