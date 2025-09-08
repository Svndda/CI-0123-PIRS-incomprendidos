#ifndef USER_H
#define USER_H

#include <fstream>
#include <qdebug.h>
#include <qlogging.h>
#include <string>
#include <vector>

/**
 * @class User
 * @brief Represents a user in the program system.
 *
 * The User class stores a user's unique identifier, name, hashed password,
 * and a set of page access permissions. It provides functionality to set and
 * verify the password as well as serialize/deserialize.
 */
class User {
public:
  /**
   * @class PageAccess
   * @brief Manages access permissions for a specific page.
   *
   * The PageAccess class holds the index of a page and its associated
   * access and data editing permissions. The permissions are represented as
   * size_t constants.
   */
  class PageAccess {
  public:
    static const size_t EDITABLE = 2;       ///< Editable data indicator.
    static const size_t NON_EDITABLE = 1;   ///< Non-editable data indicator.
    static const size_t DENIED = 0;      ///< Access denied indicator.
    
  public:
    size_t pageIndex = 0;   ///< Index of the page.
    size_t access = this->DENIED;    ///< Access permission (allowed/denegated).    
  public:
    /**
     * @brief Constructs a PageAccess object.
     * @param pageIndexed The index of the page.
     * @param pageAccess The access permission for the page.
     * @param pageDataAccess The data access permission for the page.
     */
    PageAccess(const size_t pageIndexed = 0
        , const size_t pageAccess = PageAccess::DENIED);
    
    /**
     * @brief Destructor.
     */
    ~PageAccess() = default;
  public:
    /**
     * @brief Equality operator.
     *
     * Compares two PageAccess objects for equality based on pageIndex,
     * access, and dataAccess.
     *
     * @param other The other PageAccess object to compare.
     * @return True if all attributes match; otherwise, false.
     */
    bool operator==(const PageAccess& other) const;
    
    /**
     * @brief Assignment operator.
     *
     * Copies the values from the other PageAccess into this instance.
     *
     * @param other The other PageAccess object.
     * @return Reference to the current object.
     */
    PageAccess& operator=(const PageAccess& other);
  };
    
private:
  size_t id = 0;                    ///< Unique identifier of the user.
  std::string name = "";            ///< Name of the user.
  std::vector<PageAccess> permissions; ///< Vector of page access permissions.
  size_t password = 0;              ///< Hashed password.
  
public:

  /**
   * @brief Constructs a User object.
   *
   * Initializes the User with a given id, name, and an optional set of page access permissions.
   *
   * @param userId Unique identifier for the user.
   * @param userName The user's name.
   * @param userPermissions A vector of PageAccess objects representing the user's permissions.
   */
  User(const size_t userId = 0
      , const std::string userName = ""
      , const std::vector<PageAccess> userPermissions
      = std::vector<PageAccess>());
  
  /**
   * @brief Destructor.
   */
  ~User() = default;
public:
  /**
   * @brief Equality operator.
   *
   * Compares two User objects based on their name and hashed password.
   *
   * @param other The other User object to compare.
   * @return True if the users attributes match; otherwise, false.
   */
  bool operator==(const User& other) const;
  
  /**
   * @brief Unequality operator.
   *
   * Compares two User objects based on their name and hashed password.
   *
   * @param other The other User object to compare.
   * @return True if the users attributes does not match; otherwise, false.
   */
  bool operator!=(const User& other) const;
  
  /**
   * @brief Assignment operator.
   *
   * Copies the attributes from another User into this instance.
   *
   * @param other The User object to copy from.
   * @return Reference to the current object.
   */
  User& operator=(const User& other);
  
  /**
   * @brief Retrieves the user's id.
   *
   * @return Constant reference to the user's id.
   */
  const size_t& getID() const;
  
  /**
   * @brief Sets the user's id.
   *
   * @param id The new id to set.
   */
  void setUserID(const size_t id);
  
  /**
   * @brief Retrieves the user's name.
   *
   * @return Constant reference to the user's name string.
   */
  const std::string& getUsername() const;
  
  /**
   * @brief Sets the user's name.
   *
   * @param name The new name to set.
   */
  void setUsername(const std::string name);
  
  /**
   * @brief Sets the user's password.
   *
   * Hashes the provided password string and stores it.
   *
   * @param newPassword The new password to set.
   */
  void setPassword(const std::string& newPassword);
  
  /**
   * @brief Verifies the provided password against the stored hash.
   *
   * @param passwordToCheck The password to verify.
   * @return True if the provided password matches the stored hash; otherwise, false.
   */
  bool verifyPassword(const std::string& passwordToCheck) const;
  
  /**
   * @brief Retrieves the user's permissions.
   *
   * @return Constant reference to the user's permissions.
   */
  const std::vector<PageAccess>& getUserPermissions() const;
  
  /**
   * @brief Sets the user's permissions.
   *
   * @param permissions The new permissions to set.
   */
  void setUserPermissions(const std::vector<PageAccess> permissions);
};

#endif // USER_H
