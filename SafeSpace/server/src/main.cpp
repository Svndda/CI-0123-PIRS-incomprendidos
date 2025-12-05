#include "SafeSpaceServer.h"
#include "Proxy/ProxyNode.h"
#include <csignal>
#include <iostream>
#include "nodes/interfaces/FirewallManager.h"
#include <fstream>
#include <sstream>
#include <map>
#include <filesystem>
#include <thread>
#include <vector>
#include <memory>

#include "Arduino/Arduino_Node.h"
#include "Auth/auth_udp_server.h"
#include "Bootstrap/Bootstrap.h"
#include "Bootstrap/BootstrapAdapters.h"
#include "Intermediary/IntermediaryNode.h"
#include "Storage/StorageNode.h"
#include "CriticalEvents/CriticalEventsNode.h"

static volatile std::sig_atomic_t stopFlag = 0;
extern "C" void sigHandler(int) { stopFlag = 1; }

/**
 * @brief Gets the default config directory path
 */
std::string getConfigDir() {
    std::vector<std::string> possiblePaths = {
        "config/",
        "../config/",
        "../../config/",
        "src/config/",
        "../src/config/",
        "../../src/config/"
    };

    for (const auto& path : possiblePaths) {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            return path;
        }
    }

    return "./";
}

/**
 * @brief Resolves config file path - tries multiple locations
 */
std::string resolveConfigPath(const std::string& configFile) {
    if (std::filesystem::exists(configFile)) {
        return configFile;
    }

    std::string configDir = getConfigDir();
    std::string fullPath = configDir + configFile;

    if (std::filesystem::exists(fullPath)) {
        return fullPath;
    }

    if (configFile.size() < 4 || configFile.substr(configFile.size() - 4) != ".txt") {
        fullPath = configDir + configFile + ".txt";
        if (std::filesystem::exists(fullPath)) {
            return fullPath;
        }
    }

    throw std::runtime_error("Cannot find config file: " + configFile + " in any known location");
}

/**
 * @brief Parses configuration file into key-value pairs
 */
std::map<std::string, std::string> parseConfigFile(const std::string& configPath) {
    std::map<std::string, std::string> config;
    std::string resolvedPath = resolveConfigPath(configPath);
    std::ifstream file(resolvedPath);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + resolvedPath);
    }

    std::cout << "[Main] Loading configuration from: " << resolvedPath << std::endl;

    std::string line;
    int lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (!key.empty()) {
                config[key] = value;
                std::cout << "  ▸ " << key << " = " << value << std::endl;
            }
        } else {
            std::cerr << "Warning: Invalid config line " << lineNum << ": " << line << std::endl;
        }
    }

    file.close();
    std::cout << "[Main] Loaded " << config.size() << " configuration parameters" << std::endl;
    return config;
}

/**
 * @brief Gets configuration value from config map
 */
std::string getConfigValue(const std::map<std::string, std::string>& config,
                          const std::string& key,
                          const std::string& defaultValue = "") {
    auto it = config.find(key);
    if (it != config.end()) {
        return it->second;
    }
    return defaultValue;
}

/**
 * @brief Validates and parses command-line arguments.
 */
void validateArgs(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "SafeSpace - Unified Component Launcher\n\n"
                  << "Usage:\n"
                  << "  With config file (recommended):\n"
                  << "    " << argv[0] << " <component> <config_file>\n"
                  << "    " << argv[0] << " server server.txt\n"
                  << "    " << argv[0] << " proxy proxy.txt\n"
                  << "    " << argv[0] << " bootstrap bootstrap.txt\n"
                  << "    " << argv[0] << " bootstrap-dynamic bootstrap-dynamic.txt\n\n"
                  << "  Dynamic Bootstrap Mode (NEW):\n"
                  << "    " << argv[0] << " bootstrap-dynamic <config_file>\n\n"
                  << "  With individual parameters:\n"
                  << "    " << argv[0] << " server <local_ip> <local_port> <storage_ip> <storage_port> <events_ip> <events_port> <proxy_ip> <proxy_port>\n"
                  << "    " << argv[0] << " bootstrap [ip] [port]  (sin archivo config)\n\n"
                  << "Available components: server, proxy, storage, auth, events, inter, arduino, bootstrap, bootstrap-dynamic\n"
                  << "Para activar firewall en modo server: configurar 'firewall=true' en archivo .txt\n"
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

/**
 * @brief Converts a string to a valid port number (uint16_t).
 */
uint16_t parsePort(const std::string& str) {
    try {
        int port = std::stoi(str);
        if (port <= 0 || port > 65535) {
            throw std::invalid_argument("Port out of range: " + str);
        }
        return static_cast<uint16_t>(port);
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid port number: " + str);
    }
}

/**
 * @brief Structure to hold bootstrap node configuration
 */
struct BootstrapNodeConfig {
    std::string id;
    std::string type;
    std::map<std::string, std::string> params;
};

/**
 * @brief Parses dynamic bootstrap configuration file
 */
std::vector<BootstrapNodeConfig> parseBootstrapDynamicConfig(const std::string& configFile) {
    auto config = parseConfigFile(configFile);
    std::vector<BootstrapNodeConfig> nodes;
    
    std::cout << "[Bootstrap] Parsing dynamic configuration..." << std::endl;
    
    // Find all node definitions
    int nodeIndex = 0;
    while (true) {
        std::string prefix = "node" + std::to_string(nodeIndex) + "_";
        std::string typeKey = prefix + "type";
        
        if (config.find(typeKey) == config.end()) {
            // Check if we have nodes in old format
            if (nodeIndex == 0 && config.find("node0_type") == config.end()) {
                // Try to find any node definition
                bool found = false;
                for (const auto& [key, value] : config) {
                    if (key.find("_type") != std::string::npos) {
                        found = true;
                        break;
                    }
                }
                if (!found) break;
            } else if (nodeIndex > 0) {
                break;
            }
        }
        
        BootstrapNodeConfig node;
        node.id = std::to_string(nodeIndex);
        node.type = getConfigValue(config, typeKey, "");
        
        if (node.type.empty()) {
            nodeIndex++;
            continue;
        }
        
        // Extract all parameters for this node
        for (const auto& [key, value] : config) {
            if (key.find(prefix) == 0) {
                std::string paramName = key.substr(prefix.length());
                node.params[paramName] = value;
            }
        }
        
        nodes.push_back(node);
        std::cout << "  ▸ Node " << nodeIndex << ": " << node.type << std::endl;
        nodeIndex++;
    }
    
    // If no nodes found in new format, check for bootstrap-id format
    if (nodes.empty()) {
        std::string bootstrapId = getConfigValue(config, "Bootstrap-id", "");
        if (!bootstrapId.empty()) {
            std::cout << "[Bootstrap] Found legacy format with Bootstrap-id: " << bootstrapId << std::endl;
            
            // Create nodes based on bootstrap-id
            int numNodes = 0;
            try {
                numNodes = std::stoi(bootstrapId);
            } catch (...) {
                numNodes = 4; // Default to 4 handlers
            }
            
            std::string bootstrapIp = getConfigValue(config, "Ip", "0.0.0.0");
            uint16_t bootstrapPort = parsePort(getConfigValue(config, "port", "8080"));
            
            // Create predefined handlers based on the number
            for (int i = 0; i < numNodes; i++) {
                BootstrapNodeConfig node;
                node.id = std::to_string(i);
                
                // Assign types based on position (as requested: 4 handlers)
                if (i == 0) {
                    node.type = "master";
                    node.params["local_ip"] = bootstrapIp;
                    node.params["local_port"] = std::to_string(bootstrapPort + 100); // 8180
                    node.params["storage_ip"] = "127.0.0.1";
                    node.params["storage_port"] = std::to_string(bootstrapPort + 101); // 8181
                    node.params["events_ip"] = "127.0.0.1";
                    node.params["events_port"] = std::to_string(bootstrapPort + 102); // 8182
                    node.params["proxy_ip"] = "127.0.0.1";
                    node.params["proxy_port"] = std::to_string(bootstrapPort + 103); // 8183
                } else if (i == 1) {
                    node.type = "arduino_intermediary";
                    node.params["master_ip"] = bootstrapIp;
                    node.params["master_port"] = std::to_string(bootstrapPort + 100); // 8180
                    node.params["arduino_serial"] = "simulate";
                    node.params["arduino_mode"] = "binary";
                    node.params["inter_port"] = std::to_string(bootstrapPort + 104); // 8184
                } else if (i == 2) {
                    node.type = "proxy_auth";
                    node.params["local_ip"] = bootstrapIp;
                    node.params["local_port"] = std::to_string(bootstrapPort + 105); // 8185
                    node.params["auth_ip"] = "127.0.0.1";
                    node.params["auth_port"] = std::to_string(bootstrapPort + 106); // 8186
                    node.params["master_ip"] = bootstrapIp;
                    node.params["master_port"] = std::to_string(bootstrapPort + 100); // 8180
                } else if (i == 3) {
                    node.type = "storage_events";
                    node.params["storage_local_ip"] = bootstrapIp;
                    node.params["storage_port"] = std::to_string(bootstrapPort + 107); // 8187
                    node.params["master_ip"] = bootstrapIp;
                    node.params["master_port"] = std::to_string(bootstrapPort + 100); // 8180
                    node.params["node_id"] = "storage1";
                    node.params["disk_path"] = "./storage_data";
                    node.params["events_ip"] = bootstrapIp;
                    node.params["events_port"] = std::to_string(bootstrapPort + 108); // 8188
                    node.params["events_output"] = "events.log";
                } else {
                    // Additional nodes if more than 4
                    node.type = "generic";
                    node.params["base_port"] = std::to_string(bootstrapPort + 100 + i);
                }
                
                nodes.push_back(node);
                std::cout << "  ▸ Auto-generated Node " << i << ": " << node.type << std::endl;
            }
        }
    }
    
    std::cout << "[Bootstrap] Total nodes to create: " << nodes.size() << std::endl;
    return nodes;
}

/**
 * @brief Creates node adapter based on configuration
 */
std::pair<std::function<bool()>, std::function<bool()>> createNodeAdapter(const BootstrapNodeConfig& config,
                                                                         const std::string& bootstrapIp,
                                                                         uint16_t bootstrapPort) {
    
    if (config.type == "master") {
        std::string localIp = getConfigValue(config.params, "local_ip", bootstrapIp);
        uint16_t localPort = parsePort(getConfigValue(config.params, "local_port", std::to_string(bootstrapPort + 100)));
        std::string storageIp = getConfigValue(config.params, "storage_ip", "127.0.0.1");
        uint16_t storagePort = parsePort(getConfigValue(config.params, "storage_port", std::to_string(bootstrapPort + 101)));
        std::string eventsIp = getConfigValue(config.params, "events_ip", "127.0.0.1");
        uint16_t eventsPort = parsePort(getConfigValue(config.params, "events_port", std::to_string(bootstrapPort + 102)));
        std::string proxyIp = getConfigValue(config.params, "proxy_ip", "127.0.0.1");
        uint16_t proxyPort = parsePort(getConfigValue(config.params, "proxy_port", std::to_string(bootstrapPort + 103)));
        
        return makeMasterAdapter(localIp, localPort, storageIp, storagePort,
                                 eventsIp, eventsPort, proxyIp, proxyPort);
        
    } else if (config.type == "proxy") {
        std::string localIp = getConfigValue(config.params, "local_ip", bootstrapIp);
        uint16_t localPort = parsePort(getConfigValue(config.params, "local_port", std::to_string(bootstrapPort + 110)));
        std::string authIp = getConfigValue(config.params, "auth_ip", "127.0.0.1");
        uint16_t authPort = parsePort(getConfigValue(config.params, "auth_port", std::to_string(bootstrapPort + 111)));
        std::string masterIp = getConfigValue(config.params, "master_ip", bootstrapIp);
        uint16_t masterPort = parsePort(getConfigValue(config.params, "master_port", std::to_string(bootstrapPort + 100)));
        
        return makeProxyAdapter(localIp, localPort, authIp, authPort, masterIp, masterPort);
        
    } else if (config.type == "storage") {
        uint16_t localPort = parsePort(getConfigValue(config.params, "local_port", std::to_string(bootstrapPort + 112)));
        std::string masterIp = getConfigValue(config.params, "master_ip", bootstrapIp);
        uint16_t masterPort = parsePort(getConfigValue(config.params, "master_port", std::to_string(bootstrapPort + 100)));
        std::string nodeId = getConfigValue(config.params, "node_id", "storage1");
        std::string diskPath = getConfigValue(config.params, "disk_path", "./storage_data");
        
        return makeStorageAdapter(localPort, masterIp, masterPort, nodeId, diskPath);
        
    } else if (config.type == "auth") {
        std::string localIp = getConfigValue(config.params, "local_ip", bootstrapIp);
        uint16_t localPort = parsePort(getConfigValue(config.params, "local_port", std::to_string(bootstrapPort + 113)));
        
        return makeAuthAdapter(localIp, localPort);
        
    } else if (config.type == "arduino") {
        std::string masterIp = getConfigValue(config.params, "master_ip", bootstrapIp);
        uint16_t masterPort = parsePort(getConfigValue(config.params, "master_port", std::to_string(bootstrapPort + 100)));
        std::string serialPath = getConfigValue(config.params, "serial_path", "simulate");
        std::string mode = getConfigValue(config.params, "mode", "binary");
        
        return makeArduinoAdapter(masterIp, masterPort, serialPath, mode);
        
    } else if (config.type == "events") {
        std::string localIp = getConfigValue(config.params, "local_ip", bootstrapIp);
        uint16_t localPort = parsePort(getConfigValue(config.params, "local_port", std::to_string(bootstrapPort + 114)));
        std::string outFile = getConfigValue(config.params, "output_file", "events.log");
        std::string masterIp = getConfigValue(config.params, "master_ip", bootstrapIp);
        uint16_t masterPort = parsePort(getConfigValue(config.params, "master_port", std::to_string(bootstrapPort + 100)));
        
        return makeEventsAdapter(localIp, localPort, outFile, masterIp, masterPort);
        
    } else if (config.type == "inter") {
        std::string localIp = getConfigValue(config.params, "local_ip", bootstrapIp);
        uint16_t localPort = parsePort(getConfigValue(config.params, "local_port", std::to_string(bootstrapPort + 115)));
        uint16_t masterPort = parsePort(getConfigValue(config.params, "master_port", std::to_string(bootstrapPort + 100)));
        
        return makeIntermediaryAdapter(localIp, localPort, masterPort);
        
    } else if (config.type == "arduino_intermediary") {
        // Combined handler for Arduino and Intermediary
        std::string masterIp = getConfigValue(config.params, "master_ip", bootstrapIp);
        uint16_t masterPort = parsePort(getConfigValue(config.params, "master_port", std::to_string(bootstrapPort + 100)));
        std::string serialPath = getConfigValue(config.params, "arduino_serial", "simulate");
        std::string mode = getConfigValue(config.params, "arduino_mode", "binary");
        uint16_t interPort = parsePort(getConfigValue(config.params, "inter_port", std::to_string(bootstrapPort + 116)));
        
        return std::make_pair(
            [masterIp, masterPort, serialPath, mode, interPort]() -> bool {
                try {
                    // Start Arduino node
                    ArduinoNode arduino(masterIp, masterPort, serialPath, mode);
                    // Start Intermediary node
                    IntermediaryNode inter(interPort, "0.0.0.0", masterPort);
                    
                    std::thread arduinoThread([&arduino]() { arduino.run(); });
                    std::thread interThread([&inter]() { inter.start(); });
                    
                    arduinoThread.join();
                    interThread.join();
                    return true;
                } catch (const std::exception& e) {
                    std::cerr << "[Bootstrap] Error starting arduino_intermediary: " << e.what() << std::endl;
                    return false;
                }
            },
            []() -> bool {
                std::cout << "[Bootstrap] Stopping arduino_intermediary node" << std::endl;
                return true;
            }
        );
        
    } else if (config.type == "proxy_auth") {
        // Combined handler for Proxy and Auth
        std::string localIp = getConfigValue(config.params, "local_ip", bootstrapIp);
        uint16_t localPort = parsePort(getConfigValue(config.params, "local_port", std::to_string(bootstrapPort + 117)));
        std::string authIp = getConfigValue(config.params, "auth_ip", "127.0.0.1");
        uint16_t authPort = parsePort(getConfigValue(config.params, "auth_port", std::to_string(bootstrapPort + 118)));
        std::string masterIp = getConfigValue(config.params, "master_ip", bootstrapIp);
        uint16_t masterPort = parsePort(getConfigValue(config.params, "master_port", std::to_string(bootstrapPort + 100)));
        
        return std::make_pair(
            [localIp, localPort, authIp, authPort, masterIp, masterPort]() -> bool {
                try {
                    // Start Auth server
                    AuthUDPServer auth(authIp, authPort);
                    // Start Proxy node
                    ProxyNode proxy(localIp, localPort, authIp, authPort, masterIp, masterPort);
                    
                    std::thread authThread([&auth]() { auth.serveBlocking(); });
                    std::thread proxyThread([&proxy]() { proxy.start(); });
                    
                    authThread.join();
                    proxyThread.join();
                    return true;
                } catch (const std::exception& e) {
                    std::cerr << "[Bootstrap] Error starting proxy_auth: " << e.what() << std::endl;
                    return false;
                }
            },
            []() -> bool {
                std::cout << "[Bootstrap] Stopping proxy_auth node" << std::endl;
                return true;
            }
        );
        
    } else if (config.type == "storage_events") {
        // Combined handler for Storage and Events
        std::string storageIp = getConfigValue(config.params, "storage_local_ip", bootstrapIp);
        uint16_t storagePort = parsePort(getConfigValue(config.params, "storage_port", std::to_string(bootstrapPort + 119)));
        std::string masterIp = getConfigValue(config.params, "master_ip", bootstrapIp);
        uint16_t masterPort = parsePort(getConfigValue(config.params, "master_port", std::to_string(bootstrapPort + 100)));
        std::string nodeId = getConfigValue(config.params, "node_id", "storage1");
        std::string diskPath = getConfigValue(config.params, "disk_path", "./storage_data");
        std::string eventsIp = getConfigValue(config.params, "events_ip", bootstrapIp);
        uint16_t eventsPort = parsePort(getConfigValue(config.params, "events_port", std::to_string(bootstrapPort + 120)));
        std::string eventsOutput = getConfigValue(config.params, "events_output", "events.log");
        
        return std::make_pair(
            [storagePort, masterIp, masterPort, nodeId, diskPath, eventsIp, eventsPort, eventsOutput]() -> bool {
                try {
                    // Start Storage node
                    StorageNode storage(storagePort, masterIp, masterPort, nodeId, diskPath);
                    // Start Events node
                    CriticalEventsNode events(eventsIp, eventsPort, eventsOutput);
                    events.configureMasterForwarding(masterIp, masterPort);
                    
                    std::thread storageThread([&storage]() { storage.start(); });
                    std::thread eventsThread([&events]() { events.serveBlocking(); });
                    
                    storageThread.join();
                    eventsThread.join();
                    return true;
                } catch (const std::exception& e) {
                    std::cerr << "[Bootstrap] Error starting storage_events: " << e.what() << std::endl;
                    return false;
                }
            },
            []() -> bool {
                std::cout << "[Bootstrap] Stopping storage_events node" << std::endl;
                return true;
            }
        );
    }
    
    // Default empty adapter that returns true
    return std::make_pair([]() -> bool { return true; }, []() -> bool { return true; });
}

/**
 * @brief Processes dynamic bootstrap mode
 */
int processBootstrapDynamic(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " bootstrap-dynamic <config_file>\n";
        std::cerr << "Config file format:\n";
        std::cerr << "  Bootstrap-id=<number_of_handlers>\n";
        std::cerr << "  Ip=<bootstrap_ip>\n";
        std::cerr << "  port=<bootstrap_port>\n";
        std::cerr << "  node0_type=<type>\n";
        std::cerr << "  node0_param1=value1\n";
        std::cerr << "  ...\n";
        return EXIT_FAILURE;
    }
    
    std::string configFile = argv[2];
    auto config = parseConfigFile(configFile);
    
    // Get bootstrap settings
    std::string bootstrapIp = getConfigValue(config, "Ip", "0.0.0.0");
    uint16_t bootstrapPort = parsePort(getConfigValue(config, "port", "8080"));
    
    std::cout << "[Bootstrap] Starting dynamic bootstrap on " 
              << bootstrapIp << ":" << bootstrapPort << std::endl;
    
    // Parse node configurations
    auto nodeConfigs = parseBootstrapDynamicConfig(configFile);
    
    if (nodeConfigs.empty()) {
        std::cerr << "[Bootstrap] No nodes configured. Creating default 4 handlers..." << std::endl;
        
        // Create default 4 handlers as requested
        nodeConfigs.resize(4);
        
        // Handler 1: Master server
        nodeConfigs[0].id = "0";
        nodeConfigs[0].type = "master";
        nodeConfigs[0].params["local_ip"] = bootstrapIp;
        nodeConfigs[0].params["local_port"] = std::to_string(bootstrapPort + 100);
        nodeConfigs[0].params["storage_ip"] = "127.0.0.1";
        nodeConfigs[0].params["storage_port"] = std::to_string(bootstrapPort + 101);
        nodeConfigs[0].params["events_ip"] = "127.0.0.1";
        nodeConfigs[0].params["events_port"] = std::to_string(bootstrapPort + 102);
        nodeConfigs[0].params["proxy_ip"] = "127.0.0.1";
        nodeConfigs[0].params["proxy_port"] = std::to_string(bootstrapPort + 103);
        
        // Handler 2: Arduino + Intermediary
        nodeConfigs[1].id = "1";
        nodeConfigs[1].type = "arduino_intermediary";
        nodeConfigs[1].params["master_ip"] = bootstrapIp;
        nodeConfigs[1].params["master_port"] = std::to_string(bootstrapPort + 100);
        nodeConfigs[1].params["arduino_serial"] = "simulate";
        nodeConfigs[1].params["arduino_mode"] = "binary";
        nodeConfigs[1].params["inter_port"] = std::to_string(bootstrapPort + 104);
        
        // Handler 3: Proxy + Auth
        nodeConfigs[2].id = "2";
        nodeConfigs[2].type = "proxy_auth";
        nodeConfigs[2].params["local_ip"] = bootstrapIp;
        nodeConfigs[2].params["local_port"] = std::to_string(bootstrapPort + 105);
        nodeConfigs[2].params["auth_ip"] = "127.0.0.1";
        nodeConfigs[2].params["auth_port"] = std::to_string(bootstrapPort + 106);
        nodeConfigs[2].params["master_ip"] = bootstrapIp;
        nodeConfigs[2].params["master_port"] = std::to_string(bootstrapPort + 100);
        
        // Handler 4: Storage + Events
        nodeConfigs[3].id = "3";
        nodeConfigs[3].type = "storage_events";
        nodeConfigs[3].params["storage_local_ip"] = bootstrapIp;
        nodeConfigs[3].params["storage_port"] = std::to_string(bootstrapPort + 107);
        nodeConfigs[3].params["master_ip"] = bootstrapIp;
        nodeConfigs[3].params["master_port"] = std::to_string(bootstrapPort + 100);
        nodeConfigs[3].params["node_id"] = "storage1";
        nodeConfigs[3].params["disk_path"] = "./storage_data";
        nodeConfigs[3].params["events_ip"] = bootstrapIp;
        nodeConfigs[3].params["events_port"] = std::to_string(bootstrapPort + 108);
        nodeConfigs[3].params["events_output"] = "events.log";
    }
    
    // Create bootstrap server
    Bootstrap server(bootstrapIp, bootstrapPort);
    
    // Register all nodes
    for (size_t i = 0; i < nodeConfigs.size(); i++) {
        const auto& nodeConfig = nodeConfigs[i];
        auto adapter = createNodeAdapter(nodeConfig, bootstrapIp, bootstrapPort);
        
        // Convert string ID to numeric ID (0-255)
        uint8_t nodeId;
        try {
            nodeId = static_cast<uint8_t>(std::stoi(nodeConfig.id));
        } catch (...) {
            nodeId = static_cast<uint8_t>(i);
        }
        
        server.registerNode(nodeId, adapter.first, adapter.second);
        std::cout << "[Bootstrap] Registered node ID " << (int)nodeId 
                  << " as type: " << nodeConfig.type << std::endl;
    }
    
    // Run bootstrap server
    std::thread serverThread([&server]() {
        server.serveBlocking();
    });
    
    // Interactive menu
    std::cout << "\n=== Dynamic Bootstrap Interactive Menu ===\n"
              << "Commands: list | start <id> | stop <id> | startall | stopall | quit\n"
              << "Registered " << nodeConfigs.size() << " handlers\n" << std::endl;
    
    std::string line;
    while (true) {
        std::cout << "bootstrap> ";
        if (!std::getline(std::cin, line) || stopFlag) break;
        if (line.empty()) continue;
        
        if (line == "list") {
            auto nodes = server.listNodes();
            std::cout << "Active handlers:\n";
            for (const auto& [id, running] : nodes) {
                std::cout << "  ID " << (int)id << ": " 
                          << (running ? "RUNNING" : "STOPPED") 
                          << " (Type: ";
                
                // Find the type for this ID
                for (const auto& cfg : nodeConfigs) {
                    try {
                        if (static_cast<uint8_t>(std::stoi(cfg.id)) == id) {
                            std::cout << cfg.type;
                            break;
                        }
                    } catch (...) {}
                }
                
                std::cout << ")\n";
            }
            
        } else if (line == "quit" || line == "exit") {
            break;
            
        } else if (line.rfind("start ", 0) == 0) {
            try {
                int id = std::stoi(line.substr(6));
                if (server.startNode(static_cast<uint8_t>(id))) {
                    std::cout << "Handler " << id << " started successfully." << std::endl;
                } else {
                    std::cout << "Failed to start handler " << id << std::endl;
                }
            } catch (...) {
                std::cout << "Invalid handler ID." << std::endl;
            }
            
        } else if (line.rfind("stop ", 0) == 0) {
            try {
                int id = std::stoi(line.substr(5));
                if (server.stopNode(static_cast<uint8_t>(id))) {
                    std::cout << "Handler " << id << " stopped successfully." << std::endl;
                } else {
                    std::cout << "Failed to stop handler " << id << std::endl;
                }
            } catch (...) {
                std::cout << "Invalid handler ID." << std::endl;
            }
            
        } else if (line == "startall") {
            auto nodes = server.listNodes();
            int started = 0;
            for (const auto& [id, running] : nodes) {
                if (!running) {
                    if (server.startNode(id)) {
                        std::cout << "Started handler " << (int)id << std::endl;
                        started++;
                    }
                }
            }
            std::cout << "Started " << started << " handlers." << std::endl;
            
        } else if (line == "stopall") {
            auto nodes = server.listNodes();
            int stopped = 0;
            for (const auto& [id, running] : nodes) {
                if (running) {
                    if (server.stopNode(id)) {
                        std::cout << "Stopped handler " << (int)id << std::endl;
                        stopped++;
                    }
                }
            }
            std::cout << "Stopped " << stopped << " handlers." << std::endl;
            
        } else {
            std::cout << "Unknown command. Available: list, start <id>, stop <id>, startall, stopall, quit" << std::endl;
        }
    }
    
    std::cout << "[Bootstrap] Shutting down..." << std::endl;
    server.stop();
    serverThread.join();
    
    return EXIT_SUCCESS;
}

/**
 * @brief Helper to register nodes from config file for bootstrap
 */
void registerNodesFromConfig(Bootstrap& server, const std::map<std::string, std::string>& config) {
    int nodeCount = 0;
    
    // Count how many nodes are defined in config
    for (const auto& entry : config) {
        if (entry.first.find("node") == 0 && entry.first.find("_type") != std::string::npos) {
            nodeCount++;
        }
    }
    
    std::cout << "[Bootstrap] Found " << nodeCount << " nodes in config" << std::endl;
    
    // Register each node
    for (int i = 0; i < nodeCount; i++) {
        std::string prefix = "node" + std::to_string(i) + "_";
        std::string type = getConfigValue(config, prefix + "type", "");
        
        if (type.empty()) continue;
        
        std::cout << "[Bootstrap] Registering node " << i << " (" << type << ")" << std::endl;
        
        if (type == "master") {
            std::string localIp = getConfigValue(config, prefix + "local_ip", "127.0.0.1");
            uint16_t localPort = parsePort(getConfigValue(config, prefix + "local_port", "6000"));
            std::string storageIp = getConfigValue(config, prefix + "storage_ip", "127.0.0.1");
            uint16_t storagePort = parsePort(getConfigValue(config, prefix + "storage_port", "9001"));
            std::string eventsIp = getConfigValue(config, prefix + "events_ip", "127.0.0.1");
            uint16_t eventsPort = parsePort(getConfigValue(config, prefix + "events_port", "6001"));
            std::string proxyIp = getConfigValue(config, prefix + "proxy_ip", "127.0.0.1");
            uint16_t proxyPort = parsePort(getConfigValue(config, prefix + "proxy_port", "9000"));
            
            auto p = makeMasterAdapter(localIp, localPort, storageIp, storagePort, 
                                       eventsIp, eventsPort, proxyIp, proxyPort);
            server.registerNode(i, p.first, p.second);
            
        } else if (type == "proxy") {
            std::string localIp = getConfigValue(config, prefix + "local_ip", "0.0.0.0");
            uint16_t localPort = parsePort(getConfigValue(config, prefix + "local_port", "9000"));
            std::string authIp = getConfigValue(config, prefix + "auth_ip", "127.0.0.1");
            uint16_t authPort = parsePort(getConfigValue(config, prefix + "auth_port", "7000"));
            std::string masterIp = getConfigValue(config, prefix + "master_ip", "127.0.0.1");
            uint16_t masterPort = parsePort(getConfigValue(config, prefix + "master_port", "6000"));
            
            auto p = makeProxyAdapter(localIp, localPort, authIp, authPort, masterIp, masterPort);
            server.registerNode(i, p.first, p.second);
            
        } else if (type == "storage") {
            uint16_t localPort = parsePort(getConfigValue(config, prefix + "local_port", "9001"));
            std::string masterIp = getConfigValue(config, prefix + "master_ip", "127.0.0.1");
            uint16_t masterPort = parsePort(getConfigValue(config, prefix + "master_port", "6000"));
            std::string nodeId = getConfigValue(config, prefix + "node_id", "storage1");
            std::string diskPath = getConfigValue(config, prefix + "disk_path", "./storage_data");
            
            auto p = makeStorageAdapter(localPort, masterIp, masterPort, nodeId, diskPath);
            server.registerNode(i, p.first, p.second);
            
        } else if (type == "auth") {
            std::string localIp = getConfigValue(config, prefix + "local_ip", "0.0.0.0");
            uint16_t localPort = parsePort(getConfigValue(config, prefix + "local_port", "7000"));
            
            auto p = makeAuthAdapter(localIp, localPort);
            server.registerNode(i, p.first, p.second);
            
        } else if (type == "arduino") {
            std::string masterIp = getConfigValue(config, prefix + "master_ip", "127.0.0.1");
            uint16_t masterPort = parsePort(getConfigValue(config, prefix + "master_port", "6000"));
            std::string serialPath = getConfigValue(config, prefix + "serial_path", "simulate");
            std::string mode = getConfigValue(config, prefix + "mode", "binary");
            
            auto p = makeArduinoAdapter(masterIp, masterPort, serialPath, mode);
            server.registerNode(i, p.first, p.second);
            
        } else if (type == "events") {
            std::string localIp = getConfigValue(config, prefix + "local_ip", "0.0.0.0");
            uint16_t localPort = parsePort(getConfigValue(config, prefix + "local_port", "6001"));
            std::string outFile = getConfigValue(config, prefix + "output_file", "events.log");
            std::string masterIp = getConfigValue(config, prefix + "master_ip", "127.0.0.1");
            uint16_t masterPort = parsePort(getConfigValue(config, prefix + "master_port", "6000"));
            
            auto p = makeEventsAdapter(localIp, localPort, outFile, masterIp, masterPort);
            server.registerNode(i, p.first, p.second);
            
        } else if (type == "inter") {
            std::string localIp = getConfigValue(config, prefix + "local_ip", "0.0.0.0");
            uint16_t localPort = parsePort(getConfigValue(config, prefix + "local_port", "9002"));
            uint16_t masterPort = parsePort(getConfigValue(config, prefix + "master_port", "6000"));
            
            auto p = makeIntermediaryAdapter(localIp, localPort, masterPort);
            server.registerNode(i, p.first, p.second);
        }
    }
}

/**
 * @brief Processes bootstrap with config file or command line
 */
int processBootstrap(int argc, char* argv[]) {
    std::map<std::string, std::string> config;
    std::string bindIp = "0.0.0.0";
    uint16_t bindPort = 8080;
    bool usingConfigFile = false;
    std::string configFile;

    if (argc >= 3) {
        std::string secondArg = argv[2];
        if (secondArg.size() > 4 && secondArg.substr(secondArg.size() - 4) == ".txt") {
            configFile = secondArg;
            usingConfigFile = true;
        } else {
            try {
                std::string resolved = resolveConfigPath(secondArg);
                configFile = secondArg;
                usingConfigFile = true;
            } catch (...) {
                bindIp = secondArg;
                if (argc >= 4) {
                    bindPort = parsePort(argv[3]);
                }
            }
        }
    }

    if (usingConfigFile) {
        // Check if it's a dynamic config
        auto testConfig = parseConfigFile(configFile);
        if (testConfig.find("Bootstrap-id") != testConfig.end() || 
            testConfig.find("Ip") != testConfig.end()) {
            std::cout << "[Bootstrap] Detected dynamic configuration format - redirecting to bootstrap-dynamic" << std::endl;
            return processBootstrapDynamic(argc, argv);
        }
        
        config = testConfig;
        std::cout << "[Main] Running bootstrap with config file: " << configFile << std::endl;
        
        bindIp = getConfigValue(config, "bootstrap_ip", "0.0.0.0");
        bindPort = parsePort(getConfigValue(config, "bootstrap_port", "8080"));
    }

    Bootstrap server(bindIp, bindPort);
    std::cout << "[Bootstrap] Server started on " << bindIp << ":" << bindPort << std::endl;

    if (usingConfigFile) {
        // Use the existing registration logic for old format
        registerNodesFromConfig(server, config);
    } else {
        // Default hardcoded nodes (backward compatibility)
        std::cout << "[Bootstrap] Using default hardcoded nodes" << std::endl;
        
        // ID 0: SafeSpaceServer (master)
        {
            auto p = makeMasterAdapter("127.0.0.1", 6000,
                                      "0.0.0.0", 9001,
                                      "0.0.0.0", 6001,
                                      "0.0.0.0", 9000);
            server.registerNode(0, p.first, p.second);
        }
        
        // ID 1: ProxyNode
        {
            auto p = makeProxyAdapter("0.0.0.0", 9000, "0.0.0.0", 7000, "127.0.0.1", 6000);
            server.registerNode(1, p.first, p.second);
        }

        // ID 2: StorageNode
        {
            auto p = makeStorageAdapter(9001, "127.0.0.1", 6000, "storage1", "../src/model/data/registers.bin");
            server.registerNode(2, p.first, p.second);
        }

        // ID 3: IntermediaryNode
        {
            auto p = makeIntermediaryAdapter("0.0.0.0", 9002, 6000);
            server.registerNode(3, p.first, p.second);
        }

        // ID 4: AuthUDPServer
        {
            auto p = makeAuthAdapter("0.0.0.0", 7000);
            server.registerNode(4, p.first, p.second);
        }

        // ID 5: ArduinoNode
        {
            auto p = makeArduinoAdapter("127.0.0.1", 6000, "/dev/ttyACM0", "binary");
            server.registerNode(5, p.first, p.second);
        }

        // ID 6: CriticalEventsNode
        {
            auto p = makeEventsAdapter("0.0.0.0", 6001, "logs.txt", "127.0.0.1", 6000);
            server.registerNode(6, p.first, p.second);
        }
    }

    // Run bootstrap in background
    std::thread serverThread([&server]() {
        server.serveBlocking();
    });

    // Interactive menu
    std::cout << "\nBootstrap interactive menu started. Commands:\n"
              << "  list           - List all registered nodes\n"
              << "  start <id>     - Start a specific node\n"
              << "  stop <id>      - Stop a specific node\n"
              << "  startall       - Start all nodes\n"
              << "  stopall        - Stop all nodes\n"
              << "  quit / exit    - Exit bootstrap\n" << std::endl;

    std::string line;
    while (true) {
        std::cout << "bootstrap> ";
        if (!std::getline(std::cin, line) || stopFlag) break;
        if (line.empty()) continue;
        
        if (line == "list") {
            auto nodes = server.listNodes();
            std::cout << "Registered nodes (" << nodes.size() << "):\n";
            for (const auto& [id, running] : nodes) {
                std::cout << "  ID " << (int)id << ": " 
                          << (running ? "RUNNING" : "STOPPED") << std::endl;
            }
            
        } else if (line == "quit" || line == "exit") {
            break;
            
        } else if (line.rfind("start ", 0) == 0) {
            try {
                int id = std::stoi(line.substr(6));
                if (server.startNode(id)) {
                    std::cout << "Node " << id << " started successfully." << std::endl;
                } else {
                    std::cout << "Failed to start node " << id << std::endl;
                }
            } catch (...) {
                std::cout << "Invalid node ID." << std::endl;
            }
            
        } else if (line.rfind("stop ", 0) == 0) {
            try {
                int id = std::stoi(line.substr(5));
                if (server.stopNode(id)) {
                    std::cout << "Node " << id << " stopped successfully." << std::endl;
                } else {
                    std::cout << "Failed to stop node " << id << std::endl;
                }
            } catch (...) {
                std::cout << "Invalid node ID." << std::endl;
            }
            
        } else if (line == "startall") {
            auto nodes = server.listNodes();
            for (const auto& [id, running] : nodes) {
                if (!running) {
                    server.startNode(id);
                    std::cout << "Started node " << (int)id << std::endl;
                }
            }
            
        } else if (line == "stopall") {
            auto nodes = server.listNodes();
            for (const auto& [id, running] : nodes) {
                if (running) {
                    server.stopNode(id);
                    std::cout << "Stopped node " << (int)id << std::endl;
                }
            }
            
        } else {
            std::cout << "Unknown command. Available: list, start <id>, stop <id>, startall, stopall, quit" << std::endl;
        }
    }

    std::cout << "[Bootstrap] Shutting down..." << std::endl;
    server.stop();
    serverThread.join();
    std::cout << "[Bootstrap] Stopped cleanly." << std::endl;

    return EXIT_SUCCESS;
}

/**
 * @brief Processes server component with config file or command line parameters
 */
int processServer(const std::string& type, int argc, char* argv[]) {
    std::map<std::string, std::string> config;
    std::string configFile;
    bool usingConfigFile = false;

    // Check if we're using config file mode (second argument is a config file)
    if (argc >= 3) {
        std::string secondArg = argv[2];
        // Check if it looks like a config file (has .txt or exists as file)
        if (secondArg.size() > 4 && secondArg.substr(secondArg.size() - 4) == ".txt") {
            configFile = secondArg;
            usingConfigFile = true;
        } else {
            // Also check if it exists as a file without .txt extension
            try {
                std::string resolved = resolveConfigPath(secondArg);
                configFile = secondArg;
                usingConfigFile = true;
            } catch (...) {
                // Not a config file, continue with parameter mode
            }
        }
    }

    if (usingConfigFile) {
        config = parseConfigFile(configFile);
        std::cout << "[Main] Running " << type << " component with config file: " << configFile << std::endl;
    }

    if (type == "server") {
        std::string localIp, storageIp, eventsIp, proxyIp;
        uint16_t localPort, storagePort, eventsPort, proxyPort;
        bool manageFirewall = false;

        if (usingConfigFile) {
            // Load from config file
            localIp = getConfigValue(config, "local_ip", "0.0.0.0");
            localPort = parsePort(getConfigValue(config, "local_port", "8080"));
            storageIp = getConfigValue(config, "storage_ip", "127.0.0.1");
            storagePort = parsePort(getConfigValue(config, "storage_port", "8081"));
            eventsIp = getConfigValue(config, "events_ip", "127.0.0.1");
            eventsPort = parsePort(getConfigValue(config, "events_port", "8082"));
            proxyIp = getConfigValue(config, "proxy_ip", "127.0.0.1");
            proxyPort = parsePort(getConfigValue(config, "proxy_port", "8083"));

            std::string firewallOpt = getConfigValue(config, "firewall", "false");
            manageFirewall = (firewallOpt == "true" || firewallOpt == "enable" || firewallOpt == "1");
            
            std::cout << "[Main] Firewall setting: " << firewallOpt 
                      << " -> " << (manageFirewall ? "ENABLED" : "DISABLED") << std::endl;
            
        } else if (argc == 10 || argc == 11) {
            // Original parameter-based approach
            localIp = argv[2];
            localPort = parsePort(argv[3]);
            storageIp = argv[4];
            storagePort = parsePort(argv[5]);
            eventsIp = argv[6];
            eventsPort = parsePort(argv[7]);
            proxyIp = argv[8];
            proxyPort = parsePort(argv[9]);

            if (argc == 11) {
                std::string opt = argv[10];
                manageFirewall = (opt == "--enable-firewall" || opt == "--manage-firewall");
            }
        } else {
            throw std::runtime_error("Invalid arguments for server mode. Use either config file or 8 parameters.");
        }

        SafeSpaceServer server(localIp, localPort, storageIp, storagePort, eventsIp, eventsPort, proxyIp, proxyPort);
        std::cout << "[Main] Running SafeSpaceServer on " << localIp << ":" << localPort << std::endl;

        FirewallManager fw(!manageFirewall /*dry-run by default*/);
        bool fw_enabled = false;
        if (manageFirewall) {
            // Prepare port lists: enable the ports provided to this server instance
            std::vector<uint16_t> udp_ports = { localPort, storagePort, eventsPort, proxyPort };
            std::vector<uint16_t> tcp_ports = { 22 }; // Keep SSH open
            if (!fw.enable(udp_ports, tcp_ports, true)) {
                std::cerr << "[Main] Warning: failed to enable firewall (are you root?). Continuing without firewall." << std::endl;
            } else {
                fw_enabled = true;
                std::cout << "[Main] Firewall enabled for ports: UDP ";
                for (auto port : udp_ports) std::cout << port << " ";
                std::cout << "/ TCP ";
                for (auto port : tcp_ports) std::cout << port << " ";
                std::cout << std::endl;
            }
        }

        // Run server in blocking mode for master/server component
        server.serveBlocking();

        if (stopFlag) server.stop();

        if (fw_enabled) {
            if (!fw.disable()) {
                std::cerr << "[Main] Warning: failed to disable firewall on shutdown." << std::endl;
            } else {
                std::cout << "[Main] Firewall disabled on shutdown." << std::endl;
            }
        }

        std::cout << "[Main] Server stopped cleanly." << std::endl;

    } else if (type == "events") {
        std::string localIp, outPath, masterIp;
        uint16_t localPort, masterPort;

        if (usingConfigFile) {
            localIp = getConfigValue(config, "local_ip", "0.0.0.0");
            localPort = parsePort(getConfigValue(config, "local_port", "8082"));
            outPath = getConfigValue(config, "output_file", "events.log");
            masterIp = getConfigValue(config, "master_ip", "127.0.0.1");
            masterPort = parsePort(getConfigValue(config, "master_port", "8080"));
        } else if (argc == 7) {
            localIp = argv[2];
            localPort = parsePort(argv[3]);
            outPath = argv[4];
            masterIp = argv[5];
            masterPort = parsePort(argv[6]);
        } else {
            throw std::runtime_error("Invalid arguments for events mode.");
        }

        CriticalEventsNode node(localIp, localPort, outPath);
        if (!masterIp.empty() && masterPort > 0) {
            node.configureMasterForwarding(masterIp, masterPort);
        }
        node.serveBlocking();

    } else if (type == "proxy") {
        std::string localIp, authNodeIp, masterNodeIp;
        uint16_t localPort, authNodePort, masterNodePort;

        if (usingConfigFile) {
            localIp = getConfigValue(config, "local_ip", "0.0.0.0");
            localPort = parsePort(getConfigValue(config, "local_port", "9000"));
            authNodeIp = getConfigValue(config, "auth_ip", "127.0.0.1");
            authNodePort = parsePort(getConfigValue(config, "auth_port", "9001"));
            masterNodeIp = getConfigValue(config, "master_ip", "127.0.0.1");
            masterNodePort = parsePort(getConfigValue(config, "master_port", "8080"));
        } else if (argc == 8) {
            localIp = argv[2];
            localPort = parsePort(argv[3]);
            authNodeIp = argv[4];
            authNodePort = parsePort(argv[5]);
            masterNodeIp = argv[6];
            masterNodePort = parsePort(argv[7]);
        } else {
            throw std::runtime_error("Invalid arguments for proxy mode.");
        }

        ProxyNode proxy(localIp, localPort, authNodeIp, authNodePort, masterNodeIp, masterNodePort);
        std::cout << "[Main] Running ProxyNode on " << localIp << ":" << localPort << std::endl;

        proxy.start();
        if (stopFlag) proxy.stop();
        std::cout << "[Main] ProxyNode stopped cleanly." << std::endl;

    } else if (type == "storage") {
        std::string localIp, masterIp, nodeId, diskPath;
        uint16_t localPort, masterPort;

        if (usingConfigFile) {
            localIp = getConfigValue(config, "local_ip", "0.0.0.0");
            localPort = parsePort(getConfigValue(config, "local_port", "8081"));
            masterIp = getConfigValue(config, "master_ip", "127.0.0.1");
            masterPort = parsePort(getConfigValue(config, "master_port", "8080"));
            nodeId = getConfigValue(config, "node_id", "storage1");
            diskPath = getConfigValue(config, "disk_path", "./storage_data");
        } else if (argc == 7) {
            localIp = argv[2];
            localPort = parsePort(argv[3]);
            masterIp = argv[4];
            masterPort = parsePort(argv[5]);
            diskPath = argv[6];
            nodeId = "storage1";
        } else {
            throw std::runtime_error("Invalid arguments for storage mode.");
        }

        StorageNode storage(localPort, masterIp, masterPort, nodeId, diskPath);
        storage.start();

    } else if (type == "auth") {
        std::string localIp;
        uint16_t localPort;

        if (usingConfigFile) {
            localIp = getConfigValue(config, "local_ip", "0.0.0.0");
            localPort = parsePort(getConfigValue(config, "local_port", "9001"));
        } else if (argc == 4) {
            localIp = argv[2];
            localPort = parsePort(argv[3]);
        } else {
            throw std::runtime_error("Invalid arguments for auth mode.");
        }

        AuthUDPServer server(localIp, localPort);
        std::cout << "Iniciando AuthUDPServer en " << localIp << ":" << localPort << std::endl;
        server.serveBlocking();

    } else if (type == "inter") {
        std::string masterIp, localIp;
        uint16_t masterPort, interPort;

        if (usingConfigFile) {
            masterIp = getConfigValue(config, "master_ip", "127.0.0.1");
            masterPort = parsePort(getConfigValue(config, "master_port", "8080"));
            localIp = getConfigValue(config, "local_ip", "0.0.0.0");
            interPort = parsePort(getConfigValue(config, "inter_port", "8090"));
        } else if (argc == 5) {
            localIp = argv[2];
            interPort = parsePort(argv[3]);
            masterPort = parsePort(argv[4]);
            masterIp = "127.0.0.1";
        } else {
            throw std::runtime_error("Invalid arguments for intermediary mode.");
        }

        IntermediaryNode node(interPort, localIp, masterPort);
        node.start();

        while (!stopFlag) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

    } else if (type == "arduino") {
        std::string masterIP, serialPath, mode;
        int masterPort;

        if (usingConfigFile) {
            masterIP = getConfigValue(config, "master_ip", "127.0.0.1");
            masterPort = parsePort(getConfigValue(config, "master_port", "8080"));
            serialPath = getConfigValue(config, "serial_path", "simulate");
            mode = getConfigValue(config, "mode", "binary");
        } else if (argc >= 4) {
            masterIP = argv[2];
            masterPort = parsePort(argv[3]);
            serialPath = (argc >= 5) ? argv[4] : "simulate";
            mode = (argc >= 6) ? argv[5] : "binary";
        } else {
            std::cerr << "Uso: ./safespace arduino <master_ip> <master_port> [serial_path] [mode]" << std::endl;
            return 1;
        }

        ArduinoNode node(masterIP, masterPort, serialPath, mode);
        node.run();

    } else if (type == "bootstrap-dynamic") {
        return processBootstrapDynamic(argc, argv);
        
    } else {
        throw std::runtime_error("Invalid component type: " + type);
    }

    return EXIT_SUCCESS;
}

int main(const int argc, char* argv[]) {
    validateArgs(argc, argv);

    struct sigaction sa{};
    sa.sa_handler = sigHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    try {
        std::string type = argv[1];

        if (type == "bootstrap" || type == "bootstrap-dynamic") {
            if (type == "bootstrap-dynamic") {
                return processBootstrapDynamic(argc, argv);
            } else {
                return processBootstrap(argc, argv);
            }
        } else {
            return processServer(type, argc, argv);
        }
    } catch (const std::exception& ex) {
        std::cerr << "[Fatal] " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}