#include "server.h"

int main() {
  Server::Instance();

  if (Server::Instance().InitServer()) {
    Daemon();
    Server::Instance().Loop();
  }

  return 0;
}
