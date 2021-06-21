#include "cli-params.hpp"

CliParams parse_cli(int argc, char **argv) {
  CliParams cli_params;
  CLI::App app{"nbdcpy: copy from and to nbd servers"};
  app.add_option("source port", cli_params.src_port,
                 "localhost port where source nbd server is located")
      ->required();
  app.add_option("destination port", cli_params.dest_port,
                 "localhost port where source nbd server is located")
      ->required();

  try {
    app.parse(argc, argv);
  } catch (CLI::ParseError &e) {
    app.exit(e);
    exit(1);
  }

  return cli_params;
}
