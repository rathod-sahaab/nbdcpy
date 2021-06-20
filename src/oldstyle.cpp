#include "oldstyle.hpp"
#include <bitset>
#include <cstring>
#include <iomanip>
#include <iostream>

bool is_oldstyle_offer(const OldstyleServerOffer &offer) {
  return offer.magic == NBDMAGIC and offer.cliserv_magic == CLISERVE_MAGIC;
}

void print_server_offer(const OldstyleServerOffer &server_offer) {
  char magic_string[9];
  memcpy(magic_string, &server_offer.magic, 8);

  std::cout << magic_string << "\n";
  std::cout << std::setbase(16) << std::showbase << server_offer.cliserv_magic
            << std::setbase(0) << "\n";
  std::cout << server_offer.export_size << "\n";

  std::bitset<32> bits(server_offer.flags);
  std::cout << "0b" << bits << "\n";

  return;
}
