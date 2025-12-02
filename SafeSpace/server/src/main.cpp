#include "SafeSpaceServer.h"
#include "Proxy/ProxyNode.h"
#include <csignal>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <filesystem>

#include "Arduino/Arduino_Node.h"
#include "Auth/auth_udp_server.h"
#include "Bootstrap/Bootstrap.h"
#include "Intermediary/IntermediaryNode.h"
#include "Storage/StorageNode.h"

static volatile std::sig_atomic_t stopFlag = 0;
extern "C" void sigHandler(int) { stopFlag = 1; }

/**
 * @brief Gets the default config directory path
 */
std::string getConfigDir() {
    // Try multiple possible locations
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
    
    // Fallback to current directory
    return "./";
}

/**
 * @brief Resolves config file path - tries multiple locations
 */
std::string resolveConfigPath(const std::string& configFile) {
    // If it's already an absolute path or exists in current directory, use it
    if (std::filesystem::exists(configFile)) {
        return configFile;
    }
    
    // Try in config directory
    std::string configDir = getConfigDir();
    std::string fullPath = configDir + configFile;
    
    if (std::filesystem::exists(fullPath)) {
        return fullPath;
    }
    
    // Try with .txt extension if not present
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
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        
        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);
            
            // Trim whitespace
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
 * @brief Gets configuration value from either command line or config file
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
    if (argc < 3) {
        std::cerr << "SafeSpace - Unified Component Launcher\n\n"
                  << "Usage:\n"
                  << "  With config file (recommended):\n"
                  << "    " << argv[0] << " <component> <config_file>\n"
                  << "    " << argv[0] << " server server.txt\n"
                  << "    " << argv[0] << " proxy proxy.txt\n\n"
                  << "  With individual parameters:\n"
                  << "    " << argv[0] << " server <local_ip> <local_port> <storage_ip> <storage_port> <events_ip> <events_port> <proxy_ip> <proxy_port>\n"
                  << "    " << argv[0] << " proxy <local_ip> <local_port> <auth_ip> <auth_port> <master_ip> <master_port>\n"
                  << "    " << argv[0] << " storage <local_ip> <local_port> <master_ip> <master_port> <diskPath>\n"
                  << "    " << argv[0] << " auth <local_ip> <local_port>\n"
                  << "    " << argv[0] << " events <local_ip> <local_port> <out_file>\n"
                  << "    " << argv[0] << " inter <master_ip> <master_port> <local_port>\n"
                  << "    " << argv[0] << " arduino <master_ip> <master_port> [serial_path] [mode]\n"
                  << "    " << argv[0] << " bootstrap\n\n"
                  << "Available components: server, proxy, storage, auth, events, inter, arduino, bootstrap\n"
                  << "Config files are automatically searched in: ./config/, ../config/, src/config/\n"
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
            } else if (argc == 10) {
                // Original parameter-based approach
                localIp = argv[2];
                localPort = parsePort(argv[3]);
                storageIp = argv[4];
                storagePort = parsePort(argv[5]);
                eventsIp = argv[6];
                eventsPort = parsePort(argv[7]);
                proxyIp = argv[8];
                proxyPort = parsePort(argv[9]);
            } else {
                throw std::runtime_error("Invalid arguments for server mode. Use either config file or 8 parameters.");
            }

            SafeSpaceServer server(localIp, localPort, storageIp, storagePort, eventsIp, eventsPort, proxyIp, proxyPort);
            std::cout << "[Main] Running SafeSpaceServer on " << localIp << ":" << localPort << std::endl;

            server.serveBlocking();

            if (stopFlag) server.stop();
            std::cout << "[Main] Server stopped cleanly." << std::endl;

        } else if (type == "events") {
            std::string localIp, outPath;
            uint16_t localPort;

            if (usingConfigFile) {
                localIp = getConfigValue(config, "local_ip", "0.0.0.0");
                localPort = parsePort(getConfigValue(config, "local_port", "8082"));
                outPath = getConfigValue(config, "output_file", "events.log");
            } else if (argc == 5) {
                localIp = argv[2];
                localPort = parsePort(argv[3]);
                outPath = argv[4];
            } else {
                throw std::runtime_error("Invalid arguments for events mode.");
            }

            CriticalEventsNode node(localIp, localPort, outPath);
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
                masterIp = argv[2];
                masterPort = parsePort(argv[3]);
                interPort = parsePort(argv[4]);
                localIp = "0.0.0.0";
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

        } else if (type == "bootstrap") {
            std::string localIp;
            uint16_t localPort;

            if (usingConfigFile) {
                localIp = getConfigValue(config, "local_ip", "0.0.0.0");
                localPort = parsePort(getConfigValue(config, "local_port", "8080"));
            } else {
                localIp = "0.0.0.0";
                localPort = 8080;
            }

            Bootstrap server(localIp, localPort);
            server.serveBlocking();

        } else {
            throw std::runtime_error("Invalid component type: " + type);
        }
    } catch (const std::exception& ex) {
        std::cerr << "[Fatal] " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}