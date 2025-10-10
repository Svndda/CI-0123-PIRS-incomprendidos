
#ifndef AUTH_SERVER_H
#define AUTH_SERVER_H

#include "AuthenticationRequest.h"
#include "AuthenticationResponse.h"
#include <unordered_map>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <functional>

struct User {
    std::string username;
    std::string password_hash;
    std::string salt;
    std::string group;
    int permissions;
    int failed_attempts;
    bool is_locked;
};

class AuthServer {
private:
    std::unordered_map<std::string, User> users;
    std::map<std::string, std::string> active_sessions;
    std::mutex users_mutex;
    std::mutex sessions_mutex;
    
    int server_fd;
    int port;
    bool running;
    
public:
    AuthServer(int port = 8080);
    ~AuthServer();
    
    bool start();
    void stop();
    void run();
    
    // Información del servidor
    std::string getServerIP() const;
    int getPort() const { return port; }
    
    // Gestión de usuarios
    bool addUser(const std::string& username, const std::string& password, 
                 const std::string& group, int permissions);
    
private:
    void handleClient(int client_socket, const std::string& client_ip);
    AuthenticationResponse processAuthentication(const std::string& username, 
                                               const std::string& password_hash);
    void loadDefaultUsers();
    std::string generateSessionID();
    std::string hashPassword(const std::string& password);  // Añadir esta función
};

#endif