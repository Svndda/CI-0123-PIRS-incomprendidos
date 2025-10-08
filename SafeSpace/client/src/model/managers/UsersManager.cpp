#include "UsersManager.h"

UsersManager::UsersManager(FileSystem& fs) : fileSystem(fs) {
  // Leer contenido actual del archivo
  this->fileSystem.openFile(this->userFile);
  
  loadUsers();
}

UsersManager::~UsersManager() {
    // Cleanup if necessary
  this->fileSystem.closeFile(this->userFile);  
}

bool UsersManager::saveUser(const User& user) {
    std::string username = user.getUsername();
    for (const auto& existingUser : this->users) {
        if (existingUser.getUsername() == username) {
            std::cout << "User already exists." << std::endl;
            return false;
        }
    }
    
    // Asegurarse de que el archivo de usuarios existe
    if (this->fileSystem.find(this->userFile) != 0) {
      this->fileSystem.create(this->userFile/*, "rw"*/);
    }
    

    std::string currentContent = this->fileSystem.read(this->userFile); 

    // Preparar nueva línea
    std::string userType = user.getType();
    std::string password_hash = user.getPasswordHash();
    std::string newLine = userType + "," + username + "," + password_hash + ";";

    // Concatenar
    std::string fullContent = currentContent + newLine;

    // Guardar TODO el contenido
    this->fileSystem.create(this->userFile);
    this->fileSystem.write(this->userFile, fullContent);
    // this->fileSystem.closeFile(this->userFile);

    this->users.push_back(user);
    // std::cout << "User saved successfully." << std::endl;
    // std::cout << "Registered users: " << this->users.size() << std::endl;
    // for (const auto& existingUser : this->users) {
    //   std::cout << "Registered user: " << existingUser.getUsername() << std::endl;
    // }
    return true;
  }


void UsersManager::loadUsers(){
  std::string data;
  // this->fileSystem.openFile(this->userFile);
  data = this->fileSystem.read(this->userFile);
  // this->fileSystem.closeFile(this->userFile);
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
      User user(username, userType);
      user.setPasswordHash(passwordHash); // Asumiendo que setPassword maneja hashes
      this->users.push_back(user);
    }
    start = endData + 1;
  }
  std::cout << "Users loaded successfully." << std::endl;

}
    
 
bool UsersManager::authenticate(const std::string& username, const std::string& password){
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

bool UsersManager::deleteUser(const std::string& username){
  User user = this->findUser(username);
  if (user.getUsername().empty()) {
    std::cout << "User not found" << std::endl;
    return false;
  }

  // this->fileSystem.openFile(this->userFile);
  std::string currentContent = this->fileSystem.read(this->userFile); 
  // this->fileSystem.closeFile(this->userFile);

  std::string newContent;
  size_t start = 0;
  bool found = false;
  while (true) {
    size_t endData = currentContent.find(';', start);
    if (endData == std::string::npos) break;
    std::string userEntry = currentContent.substr(start, endData - start);
    size_t firstComma = userEntry.find(',');
    size_t secondComma = userEntry.find(',', firstComma + 1);
    std::string entryUsername;
    if (firstComma != std::string::npos && secondComma != std::string::npos) {
      entryUsername = userEntry.substr(firstComma + 1, secondComma - firstComma - 1);
    }

    // Si no es el usuario a eliminar, lo agrego al nuevo contenido
    if (entryUsername != username) {
        newContent += userEntry + ";";
    } else {
        found = true;
    }
    start = endData + 1;
  }

  // Actualiza el archivo
  // this->fileSystem.openFile(this->userFile);
  this->fileSystem.write(this->userFile, newContent);
  // this->fileSystem.closeFile(this->userFile);

  // Elimina del vector
  for (auto it = this->users.begin(); it != this->users.end(); ++it) {
      if (it->getUsername() == username) {
          this->users.erase(it);
          break;
      }
  }

  if (found) {
      std::cout << "User deleted successfully." << std::endl;
      return true;
  } else {
      std::cout << "User not found in file." << std::endl;
      return false;
  }
}
bool UsersManager::updateUser(const std::string& username, const User& updatedUser){
  for (auto& user : this->users) {
    if (user.getUsername() == username) {
      user = updatedUser;
      // Actualizar en el archivo
      // this->fileSystem.openFile(this->userFile);
      std::string currentContent = this->fileSystem.read(this->userFile); 
      // this->fileSystem.closeFile(this->userFile);

      std::string newContent;
      size_t start = 0;
      while (true) {
        size_t endData = currentContent.find(';', start);
        if (endData == std::string::npos) break;
        std::string userEntry = currentContent.substr(start, endData - start);
        size_t firstComma = userEntry.find(',');
        std::string entryUsername;
        if (firstComma != std::string::npos) {
          entryUsername = userEntry.substr(0, firstComma);
        }
        if (entryUsername == username) {
          // Reemplazar con los datos actualizados
          std::string userType = updatedUser.getType();
          std::string password_hash = updatedUser.getPasswordHash();
          newContent += username + "," + userType + "," + password_hash + ";";
        } else {
          newContent += userEntry + ";";
        }
        start = endData + 1;
      }

      // Guardar el nuevo contenido
      // this->fileSystem.openFile(this->userFile);
      this->fileSystem.write(this->userFile, newContent);
      // this->fileSystem.closeFile(this->userFile);

      std::cout << "User updated successfully." << std::endl;
      return true;
    }
  }
  std::cout << "User not found." << std::endl;
  return false;
}
  
User UsersManager::findUser(const std::string& username){
  for (const auto& user : this->users) {
    if (user.getUsername() == username) {
      return user;
    }
  }
  std::cout << "User not found." << std::endl;
  return User();
}

