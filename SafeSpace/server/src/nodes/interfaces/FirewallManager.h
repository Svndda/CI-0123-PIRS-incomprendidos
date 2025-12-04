// FirewallManager: simple wrapper to manage iptables via system commands.
// Provides enable/disable/status/save functionality.

#ifndef SERVER_FIREWALLMANAGER_H
#define SERVER_FIREWALLMANAGER_H

#include <cstdint>
#include <string>
#include <vector>

class FirewallManager {
public:
  explicit FirewallManager(bool dryRun = false) noexcept;
  ~FirewallManager() = default;

  // Apply firewall rules for given UDP and TCP ports. Returns true on success.
  bool enable(const std::vector<uint16_t>& udpPorts,
              const std::vector<uint16_t>& tcpPorts,
              bool allowIcmp = true) const;

  // Remove firewall rules (flush) and set permissive policies.
  bool disable() const;

  // Return textual status (iptables -L ...). Empty string on failure.
  std::string status() const;

  // Save rules using iptables-save to the given path (e.g. /etc/iptables/rules.v4)
  bool saveRules(const std::string& path) const;

private:
  bool runCmd(const std::string& cmd, std::string* out = nullptr) const;
  bool isRoot() const;
  bool dryRun_;
};

#endif // SERVER_FIREWALLMANAGER_H
