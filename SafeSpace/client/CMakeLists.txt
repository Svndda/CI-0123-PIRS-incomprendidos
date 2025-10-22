cmake_minimum_required(VERSION 3.19)
project(SafeSpace LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS
  Core Widgets
  LinguistTools
  PrintSupport
  Svg
  Network
)

find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)

qt_standard_project_setup()

qt_add_executable(SafeSpace
  WIN32 MACOSX_BUNDLE
  src/main.cpp
  
  # Model
  src/model/model.cpp src/model/model.h
  
  # Managers
  src/model/managers/UsersManager.cpp  src/model/managers/UsersManager.h
  
  # Structures
  src/model/structures/user.cpp src/model/structures/user.h
  src/model/structures/sensordata.h src/model/structures/sensordata.cpp

  # Filesystem  



  src/model/filesystem/iNode.h
  
  # Controller
  src/controller/appcontroller.cpp src/controller/appcontroller.h
  src/controller/mainwindow.ui
  
  src/view/loginpage.cpp src/view/loginpage.h src/view/loginpage.ui
  src/view/page.h src/view/page.cpp
  src/colors.h
  src/view/elements/button.h src/view/elements/button.cpp
  src/view/elements/sensordatafield.h src/view/elements/sensordatafield.cpp src/view/elements/sensordatafield.ui
  src/view/administrationpage.h src/view/administrationpage.cpp src/view/administrationpage.ui
  src/view/registrationpage.h src/view/registrationpage.cpp src/view/registrationpage.ui
  src/view/systemuserspage.h src/view/systemuserspage.cpp src/view/systemuserspage.ui
  
  src/view/accountpage.h src/view/accountpage.cpp src/view/accountpage.ui
  src/view/visualizerspage.h src/view/visualizerspage.cpp src/view/visualizerspage.ui
  src/view/sensorspage.h src/view/sensorspage.cpp src/view/sensorspage.ui
  src/view/elements/rolescombobox.h src/view/elements/rolescombobox.cpp
  src/model/filesystem/Layout.h
  src/model/filesystem/DiskManager.h src/model/filesystem/DiskManager.cpp
  src/model/filesystem/FileSystem.h src/model/filesystem/FileSystem.cpp
  src/model/filesystem/DirEntry.h
  src/model/data/unity.bin
  src/model/network/authenticationrequest.h
  src/model/network/authenticationresponse.h
  
  src/model/network/qtudpclient.h src/model/network/qtudpclient.cpp
  

)

qt_add_translations(
  TARGETS SafeSpace
  TS_FILES SafeSpace_es_CR.ts
)

target_link_libraries(SafeSpace
  PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Svg
    Qt6::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

target_include_directories(SafeSpace
  PRIVATE
    ${OPENSSL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/src/model/
    ${CMAKE_SOURCE_DIR}/src/model/structures
    ${CMAKE_SOURCE_DIR}/src/model/filesystem    
    ${CMAKE_SOURCE_DIR}/src/controller/
    ${CMAKE_SOURCE_DIR}/src/ui/
    ${CMAKE_SOURCE_DIR}/src/ui/elements/
    
)

include(GNUInstallDirs)

install(TARGETS SafeSpace
  BUNDLE  DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
  TARGET SafeSpace
  OUTPUT_SCRIPT deploy_script
  NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
