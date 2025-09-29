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

    // Leer contenido actual del archivo
    this->fileSystem->openFile(this->userFile);
    std::string currentContent = this->fileSystem->readFileAsString(this->userFile); 
    this->fileSystem->closeFile(this->userFile);

    // Preparar nueva línea
    std::string password_hash = std::to_string(user.getPasswordHash());
    std::string newLine = username + "," + password_hash + ";";

    // Concatenar
    std::string fullContent = currentContent + newLine;

    // Guardar TODO el contenido
    this->fileSystem->openFile(this->userFile);
    this->fileSystem->writeFile(this->userFile, fullContent);
    this->fileSystem->closeFile(this->userFile);

    this->users.push_back(user);
    std::cout << "User saved successfully." << std::endl;
  }


  void loadUsers(){
    
  }
  bool authenticate(const std::string& username, const std::string& password);
  void listUsers();
  bool deleteUser(const std::string& username);
  bool updateUser(const std::string& username, const User& updatedUser);
  User getUser(const std::string& username);
