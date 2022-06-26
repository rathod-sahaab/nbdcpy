#ifndef NBDCPY_UI_OUTUPUTS_HPP
#define NBDCPY_UI_OUTUPUTS_HPP

#include "../proto/oldstyle.hpp"
#include <string>

namespace NBDCpy {
namespace Outputs {
void print_header(const std::string &s);

void print_server_offer(const OldstyleServerOffer &server_offer);
} // namespace Outputs
} // namespace NBDCpy

#endif // NBDCPY_UI_OUTUPUTS_HPP
