#include "cli-params.hpp"
#include "nbd_connection.hpp"
#include "oldstyle.hpp"
#include <cstdlib>

int main(int argc, char **argv) {

  const auto cli_params = parse_cli(argc, argv);
  std::cout << cli_params.src_port << " --> " << cli_params.dest_port << "\n";

  NbdConnection nbd_src(cli_params.src_port), nbd_dest(cli_params.dest_port);

  if (nbd_dest.get_size() < nbd_src.get_size()) {
    std::cout << "Source is larger than destination, will cause data loss"
              << "\n";
    exit(1);
  }

  return 0;
}

// ------------------ helpers ------------------
void print_error(const char *err) {}
