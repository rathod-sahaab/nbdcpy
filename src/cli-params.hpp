#ifndef CLI_OPTS_HPP
#define CLI_OPTS_HPP

#include "CLI/App.hpp"
#include "CLI/Config.hpp"
#include "CLI/Formatter.hpp"

struct CliParams {
  int src_port;
  int dest_port;
  off_t max_pakcket_size;
  off_t max_inflight_requests;
};

CliParams parse_cli(int argc, char **argv);

void display_info(const CliParams &cli_params);
#endif // CLI_OPTS_HPP
