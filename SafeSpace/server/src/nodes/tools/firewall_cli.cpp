#include "../interfaces/FirewallManager.h"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: firewall_cli <enable|disable|status> [--dry-run]\n";
    return 1;
  }

  std::string cmd = argv[1];
  bool dry = false;
  for (int i = 2; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--dry-run") dry = true;
  }

  FirewallManager fw(dry);

  // Default ports discovered in the project
  std::vector<uint16_t> udp_ports = {6000, 7000, 9000, 9001, 9002, 6001};
  std::vector<uint16_t> tcp_ports = {22};

  if (cmd == "enable") {
    bool ok = fw.enable(udp_ports, tcp_ports, true);
    std::cout << (ok ? "Firewall enabled" : "Failed to enable firewall") << std::endl;
    return ok ? 0 : 2;
  }
  if (cmd == "disable") {
    bool ok = fw.disable();
    std::cout << (ok ? "Firewall disabled" : "Failed to disable firewall") << std::endl;
    return ok ? 0 : 2;
  }
  if (cmd == "status") {
    std::string s = fw.status();
    if (s.empty()) {
      std::cerr << "Failed to get iptables status (are you root?)\n";
      return 2;
    }
    std::cout << s << std::endl;
    return 0;
  }

  std::cerr << "Unknown command: " << cmd << "\n";
  return 1;
}
