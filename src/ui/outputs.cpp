#include "outputs.hpp"
#include "fmt/core.h"
#include <algorithm>
#include <bitset>

namespace NBDCpy {
namespace Outputs {
void print_header(const std::string &s) {
  fmt::print("┌{0:─^{2}}┐\n"
             "│{1: ^{2}}│\n"
             "└{0:─^{2}}┘\n",
             "", s, 30);
}

void print_server_offer(const OldstyleServerOffer &server_offer) {
  char magic_string[9];
  memcpy(magic_string, &server_offer.magic, 8);

  if (magic_string[0] == 'C') {
    // if machine is little_endian the last character C becomes the first
    // character. So we reverse to see NBDMAGIC
    std::reverse(magic_string, magic_string + 8);
  }

  fmt::print("{} ({:#x})\n", magic_string, server_offer.magic);
  fmt::print("{:#x}\n", server_offer.cliserv_magic);
  fmt::print("{}\n", server_offer.export_size);
  const auto gbits = std::bitset<16>(server_offer.gflags);
  fmt::print("0b{}\n", gbits.to_string());
  const auto ebits = std::bitset<16>(server_offer.eflags);
  fmt::print("0b{}\n", ebits.to_string());

  return;
}
} // namespace Outputs
} // namespace NBDCpy
