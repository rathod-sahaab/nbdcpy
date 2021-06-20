#ifndef CLI_OPTS_HPP
#define CLI_OPTS_HPP

#include "CLI/App.hpp"
#include "CLI/Config.hpp"
#include "CLI/Formatter.hpp"

struct CliParams {
  int src_port, dest_port;
};

CliParams parse_cli(int argc, char **argv);
#endif // CLI_OPTS_HPP
