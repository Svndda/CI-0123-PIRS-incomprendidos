cmake_minimum_required(VERSION 3.19)
project(SafeSpace LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS
  Core Widgets
  LinguistTools
  PrintSupport
  Svg
  Network
)

find_package(REQUIRED COMPONENTS SSL Crypto)

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
  src/model/structures/sensordata.h
  src/model/structures/sensordata.cpp

  # Filesystem  
  src/model/filesystem/iNode.h
  
  # Controller
  src/controller/appcontroller.cpp src/controller/appcontroller.h
  src/controller/mainwindow.ui
  
  src/view/loginpage.cpp src/view/loginpage.h src/view/loginpage.ui
  src/view/page.h src/view/page.cpp
  src/colors.h
  src/view/elements/button.h src/view/elements/button.cpp
  
  src/view/administration/administrationpage.h
  src/view/administration/administrationpage.cpp
  src/view/administration/administrationpage.ui
  src/view/administration/subpages/registrationpage.h
  src/view/administration/subpages/registrationpage.cpp
  src/view/administration/subpages/registrationpage.ui
  src/view/administration/subpages/systemuserspage.h
  src/view/administration/subpages/systemuserspage.cpp
  src/view/administration/subpages/systemuserspage.ui
  
  src/view/account/accountpage.h
  src/view/account/accountpage.cpp
  src/view/account/accountpage.ui
  src/view/account/subpages/settings.h
  src/view/account/subpages/settings.cpp
  src/view/account/subpages/settings.ui
  
  src/view/arduinos/arduinospage.h
  src/view/arduinos/arduinospage.cpp
  src/view/arduinos/arduinospage.ui
  src/view/arduinos/subpages/sensorspage.h
  src/view/arduinos/subpages/sensorspage.cpp
  src/view/arduinos/subpages/sensorspage.ui
  src/view/arduinos/subpages/machinespage.h
  src/view/arduinos/subpages/machinespage.cpp
  src/view/arduinos/subpages/machinespage.ui
  
  src/view/elements/rolescombobox.h
  src/view/elements/rolescombobox.cpp
  
  src/model/filesystem/Layout.h
  
  src/model/filesystem/DiskManager.h
  src/model/filesystem/DiskManager.cpp
  
  src/model/filesystem/FileSystem.h
  src/model/filesystem/FileSystem.cpp
  
  src/model/filesystem/DirEntry.h
  src/model/data/unity.bin
  src/model/network/authenticationrequest.h
  src/model/network/authenticationresponse.h
  src/model/network/connectrequest.cpp
  src/model/network/connectrequest.h
  src/model/network/disconnectrequest.cpp
  src/model/network/disconnectrequest.h
  
  src/model/network/qtudpclient.h
  src/model/network/qtudpclient.cpp
  src/view/arduinos/subpages/registerspage.h
  src/view/arduinos/subpages/registerspage.cpp
  src/view/arduinos/subpages/registerspage.ui
  
  src/view/nodes/nodespage.h
  src/view/nodes/nodespage.cpp
  src/view/nodes/nodespage.ui
  src/model/network/ConnectResponse.h
  src/model/network/disconnectresponse.h
  src/model/network/connectresponse.h
  # ../common/LogManager.h ../common/LogManager.cpp

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
    ${CMAKE_SOURCE_DIR}/src/view/
    ${CMAKE_SOURCE_DIR}/src/view/elements/    
    ${CMAKE_SOURCE_DIR}/src/view/arduinos/
    ${CMAKE_SOURCE_DIR}/src/view/arduinos/subpages/
    ${CMAKE_SOURCE_DIR}/src/view/administration/
    ${CMAKE_SOURCE_DIR}/src/view/administration/subpages/
    ${CMAKE_SOURCE_DIR}/src/view/account/
    ${CMAKE_SOURCE_DIR}/src/view/account/subpages/
    ${CMAKE_SOURCE_DIR}/../common
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
