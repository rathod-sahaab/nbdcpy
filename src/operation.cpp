#include "operation.hpp"

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
  default:
    return '-';
  }
}
