#include "cli-params.hpp"
#include <fmt/core.h>

CliParams parse_cli(int argc, char **argv) {
  constexpr const off_t MAX_PACKET_SIZE = 512; // bytes
  constexpr const int MAX_INFLIGHT_REQUESTS = 16;

  CliParams cli_params;
  CLI::App app{"nbdcpy: copy from and to nbd servers"};
  app.add_option("source port", cli_params.src_port,
                 "localhost port where source nbd server is located")
      ->required();
  app.add_option("destination port", cli_params.dest_port,
                 "localhost port where source nbd server is located")
      ->required();
  app.add_option("max packet size", cli_params.max_pakcket_size, "")
      ->default_val(MAX_PACKET_SIZE);
  app.add_option("max inflight requests", cli_params.max_inflight_requests,
                 "Maximum number of request allowed to be in flight (not "
                 "completed) simultaneously")
      ->default_val(MAX_INFLIGHT_REQUESTS);

  try {
    app.parse(argc, argv);
  } catch (CLI::ParseError &e) {
    app.exit(e);
    exit(1);
  }

  return cli_params;
}

void display_info(const CliParams &cli_params) {
  fmt::print("Copying from port {} to port {}.\n", cli_params.src_port,
             cli_params.dest_port);
  fmt::print("Max packet size: {}\n", cli_params.max_pakcket_size);
  fmt::print("Max inflight requests: {}\n", cli_params.max_inflight_requests);
}
