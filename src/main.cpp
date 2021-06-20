#include "nbd_connection.hpp"
#include "oldstyle.hpp"

int main(int argc, char **argv) {
  const int port = 2345;
  NbdConnection nbd_src(port);

  return 0;
}

// ------------------ helpers ------------------
void print_error(const char *err) {}
