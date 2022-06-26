#include "oldstyle.hpp"
#include "fmt/format.h"
#include <algorithm>
#include <bitset>
#include <cstring>
#include <iomanip>
#include <iostream>

bool is_oldstyle_offer(const OldstyleServerOffer &offer) {
  return offer.magic == NBDMAGIC and offer.cliserv_magic == CLISERVE_MAGIC;
}

