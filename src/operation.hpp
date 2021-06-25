#ifndef OPERATION_HPP
#define OPERATION_HPP
#include <sys/types.h>

enum class OperationState : u_int32_t {
  EMPTY,
  REQUESTING,
  READING,
  WRITING,
  CONFIRMING,
};

struct Operation {
  u_int64_t offset;
  u_int32_t length;
  OperationState state;
} __attribute__((__packed__));

#endif // OPERATION_HPP

