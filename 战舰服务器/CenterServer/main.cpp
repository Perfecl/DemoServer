#include <iostream>
#include <string>
#include "system.h"
#include "server.h"

int main(int argc, char* argv[]) {
  Server::Instance();
  if (Server::Instance().InitServer()) {
    Daemon();

    Server::Instance().Loop();
  }

  return 0;
}
