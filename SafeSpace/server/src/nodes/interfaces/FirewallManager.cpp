#include "FirewallManager.h"

#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>
#include <sstream>
#include <unistd.h>

FirewallManager::FirewallManager(bool dryRun) noexcept
  : dryRun_(dryRun) {}

bool FirewallManager::isRoot() const {
  return (::geteuid() == 0);
}

bool FirewallManager::runCmd(const std::string& cmd, std::string* out) const {
  if (dryRun_) {
    // Print command instead of executing in dry-run mode
    if (out) *out = "(dry-run) " + cmd + "\n";
    return true;
  }

  // Use popen to capture stdout
  std::array<char, 128> buffer;
  std::string result;
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) return false;
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
    result += buffer.data();
  }
  int rc = pclose(pipe);
  if (out) *out = result;
  return (rc == 0);
}

bool FirewallManager::enable(const std::vector<uint16_t>& udpPorts,
                             const std::vector<uint16_t>& tcpPorts,
                             bool allowIcmp) const {
  if (!isRoot() && !dryRun_) return false;

  // Basic sequence of commands (idempotent-ish): flush, default policies, accept essentials
  std::vector<std::string> cmds;
  cmds.push_back("iptables -F");
  cmds.push_back("iptables -X");
  cmds.push_back("iptables -t nat -F");
  cmds.push_back("iptables -t mangle -F");

  cmds.push_back("iptables -P INPUT DROP");
  cmds.push_back("iptables -P FORWARD DROP");
  cmds.push_back("iptables -P OUTPUT ACCEPT");

  cmds.push_back("iptables -A INPUT -i lo -j ACCEPT");
  cmds.push_back("iptables -A INPUT -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT");

  for (uint16_t p : tcpPorts) {
    std::ostringstream ss;
    ss << "iptables -A INPUT -p tcp --dport " << p << " -m conntrack --ctstate NEW -j ACCEPT";
    cmds.push_back(ss.str());
  }
  for (uint16_t p : udpPorts) {
    std::ostringstream ss;
    ss << "iptables -A INPUT -p udp --dport " << p << " -m conntrack --ctstate NEW -j ACCEPT";
    cmds.push_back(ss.str());
  }

  if (allowIcmp) cmds.push_back("iptables -A INPUT -p icmp -j ACCEPT");

  for (const auto& c : cmds) {
    std::string out;
    if (!runCmd(c, &out)) {
      return false;
    }
  }
  return true;
}

bool FirewallManager::disable() const {
  if (!isRoot() && !dryRun_) return false;
  std::vector<std::string> cmds = {
    "iptables -F",
    "iptables -X",
    "iptables -t nat -F",
    "iptables -t mangle -F",
    "iptables -P INPUT ACCEPT",
    "iptables -P FORWARD ACCEPT",
    "iptables -P OUTPUT ACCEPT"
  };
  for (const auto& c : cmds) {
    if (!runCmd(c)) return false;
  }
  return true;
}

std::string FirewallManager::status() const {
  std::string out;
  if (!runCmd("iptables -L -n -v --line-numbers", &out)) return std::string();
  return out;
}

bool FirewallManager::saveRules(const std::string& path) const {
  if (!isRoot() && !dryRun_) return false;
  std::ostringstream ss;
  ss << "iptables-save > '" << path << "'";
  std::string o;
  return runCmd(ss.str(), &o);
}
