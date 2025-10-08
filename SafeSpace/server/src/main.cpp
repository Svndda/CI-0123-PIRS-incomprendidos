#include "SafeSpaceServer.h"
#include <csignal>
#include <iostream>

static volatile std::sig_atomic_t stopFlag = 0;
extern "C" void sigHandler(int) { stopFlag = 1; }

int main() {
  struct sigaction sa{};
  sa.sa_handler = sigHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);

  try {
    const uint16_t port = 5000;
    SafeSpaceServer server(port);

    // Register a discover target
    server.addDiscoverTarget("127.0.0.1", 6000);

    std::cout << "SafeSpaceServer running (PID " << getpid() << "). Ctrl+C to stop." << std::endl;

    // Run blocking main loop
    server.serveBlocking();

    if (stopFlag) server.stop();

    std::cout << "Server shutting down." << std::endl;
  } catch (const std::exception& ex) {
    std::cerr << "Fatal: " << ex.what() << std::endl;
    return 1;
  }
  return 0;
}
