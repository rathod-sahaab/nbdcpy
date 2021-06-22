#include "utils.hpp"
#include "fmt/core.h"

void print_header(const std::string &s) {
  fmt::print("┌{0:─^{2}}┐\n"
             "│{1: ^{2}}│\n"
             "└{0:─^{2}}┘\n",
             "", s, 30);
}
