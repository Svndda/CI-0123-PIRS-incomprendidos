#ifndef AUTH_UDP_SERVER_H
#define AUTH_UDP_SERVER_H

#include <map>
#include <mutex>
#include <string>
#include <unordered_map>

#include "user.h"
#include "../../model/structures/authenticationrequest.h"
#include "../../model/structures/authenticationresponse.h"
#include "../interfaces/UDPServer.h"

// struct User {
//     std::string username;
//     std::string password_hash;
//     std::string group;
//     int permissions;
//     int failed_attempts;
//     bool is_locked;
// };

class AuthUDPServer : public UDPServer {
private:
    std::unordered_map<std::string, User> users;
    std::map<std::string, std::string> active_sessions;
    std::mutex users_mutex;
    std::mutex sessions_mutex;

public:
    AuthUDPServer(uint16_t port);
    ~AuthUDPServer();

    bool addUser(const std::string& username, const std::string& password, 
                 const std::string& group, int permissions);

protected:
    void onReceive(const sockaddr_in& peer, const uint8_t* data, ssize_t len,
                   std::string& out_response) override;

private:
    void loadDefaultUsers();
    std::string hashPassword(const std::string& password);
    std::string generateSessionID();
    void handleDiscover(const sockaddr_in& peer, const uint8_t* data, ssize_t len, 
                       std::string& out_response);
    void handleAuthRequest(const sockaddr_in& peer, const uint8_t* data, ssize_t len, 
                          std::string& out_response);
};

#endif