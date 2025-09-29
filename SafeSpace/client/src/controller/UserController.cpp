#include "UserController.h"

UserController::UserController(FileSystem* fs) : fileSystem(fs) {
    //loadUsers();
}

UserController::~UserController() {
    // Cleanup if necessary
}

void UserController::saveUser(const User& user) {
    std::string username = user.getUsername();
    for (const auto& existingUser : this->users) {
        if (existingUser.getUsername() == username) {
            std::cout << "User already exists." << std::endl;
            return;
        }
    }
    // Asegurarse de que el archivo de usuarios existe
    if (this->fileSystem->openFile(this->userFile) != 0) {
        this->fileSystem->createFile(this->userFile, "rw");
    }
    // Leer contenido actual del archivo
    this->fileSystem->openFile(this->userFile);
    std::string currentContent = this->fileSystem->readFileAsString(this->userFile); 
    this->fileSystem->closeFile(this->userFile);

    // Preparar nueva línea
    std::string userType = user.getType();
    std::string password_hash = user.getPasswordHash();
    std::string newLine = userType + "," + username + "," + password_hash + ";";

    // Concatenar
    std::string fullContent = currentContent + newLine;

    // Guardar TODO el contenido
    this->fileSystem->openFile(this->userFile);
    this->fileSystem->writeFile(this->userFile, fullContent);
    this->fileSystem->closeFile(this->userFile);

    this->users.push_back(user);
    std::cout << "User saved successfully." << std::endl;
  }


  void UserController::loadUsers(){
    std::string data;
    this->fileSystem->openFile(this->userFile);
    data = this->fileSystem->readFileAsString(this->userFile);
    this->fileSystem->closeFile(this->userFile);
    if (data.empty()) {
        std::cout << "No users found." << std::endl;
        return;
    }
    size_t start = 0;
    std::string userType;
    std::string username;
    std::string passwordHash;

    while (true) {
      size_t endData = data.find(';', start);
      // If no more entries, break the loop
      if (endData == std::string::npos) break;
      std::string userEntry = data.substr(start, endData - start);
      size_t firstComma = userEntry.find(',');
      size_t secondComma = userEntry.find(',', firstComma + 1);
      if (firstComma != std::string::npos && secondComma != std::string::npos) {
        userType = userEntry.substr(0, firstComma);
        username = userEntry.substr(firstComma + 1, secondComma - firstComma - 1);
        passwordHash = userEntry.substr(secondComma + 1);
        User user(0, username, userType);
        user.setPassword(passwordHash); // Asumiendo que setPassword maneja hashes
        this->users.push_back(user);
      }
      start = endData + 1;
    }
    std::cout << "Users loaded successfully." << std::endl;

  }
    
 
  bool authenticate(const std::string& username, const std::string& password){
    for (const auto& user : this->users) {
      if (user.getUsername() == username) {
        if (user.verifyPassword(password)) {
          std::cout << "Authentication successful." << std::endl;
          return true;
        } else {
          std::cout << "Incorrect password." << std::endl;
          return false;
        }
      }
    }
    std::cout << "User not found." << std::endl;
    return false;
  }
  
  void UserController::listUsers(){
      for(int i = 0; i < this->users.size(); ++i){
          std::cout << this->users[i].getUsername() << " " << this->users[i].getType() << "\n";
      }
  }

  bool deleteUser(const std::string& username);
  bool updateUser(const std::string& username, const User& updatedUser);
  User getUser(const std::string& username);
