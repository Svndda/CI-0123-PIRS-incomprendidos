cmake_minimum_required(VERSION 3.19)
project(SafeSpace LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS
  Core Widgets
  LinguistTools
  Network
  UiTools
)

find_package(REQUIRED COMPONENTS SSL Crypto)

qt_standard_project_setup()

qt_add_executable(SafeSpace
  WIN32 MACOSX_BUNDLE
  src/main.cpp
  
  # Model
  src/model/model.cpp src/model/model.h
  
  # Structures
  src/model/structures/user.cpp src/model/structures/user.h

  # Filesystem  
  src/model/filesystem/iNode.h
  
  # Controller
  src/controller/appcontroller.cpp src/controller/appcontroller.h
  src/controller/mainwindow.ui
  
  src/view/loginpage.cpp src/view/loginpage.h src/view/loginpage.ui
  src/view/page.h src/view/page.cpp
  src/view/elements/button.h src/view/elements/button.cpp
  
  src/model/filesystem/Layout.h
  
  src/model/filesystem/DiskManager.h
  src/model/filesystem/DiskManager.cpp
  
  src/model/filesystem/FileSystem.h
  src/model/filesystem/FileSystem.cpp
  
  src/model/filesystem/DirEntry.h
  src/model/data/unity.bin
  
  src/model/network/RunNode.h
  src/model/network/RunNodeResponse.h
  src/model/network/StopNode.h
  src/model/network/StopNodeResponse.h
  
  # src/model/network/authenticationrequest.h
  # src/model/network/authenticationresponse.h
  # src/model/network/connectrequest.cpp
  # src/model/network/connectrequest.h
  # src/model/network/disconnectrequest.cpp
  # src/model/network/disconnectrequest.h
  # src/model/network/ConnectResponse.h
  # src/model/network/disconnectresponse.h
  # src/model/network/connectresponse.h
  
  src/model/network/qtudpclient.h
  src/model/network/qtudpclient.cpp
  
  src/view/nodes/nodespage.h
  src/view/nodes/nodespage.cpp
  src/view/nodes/nodespage.ui
  src/model/data/nodes_config.text
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
    Qt6::Network
    Qt6::UiTools    
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
    ${CMAKE_SOURCE_DIR}/src/model/network
    ${CMAKE_SOURCE_DIR}/src/controller/
    ${CMAKE_SOURCE_DIR}/src/view/
    ${CMAKE_SOURCE_DIR}/src/view/elements/
    ${CMAKE_SOURCE_DIR}/src/view/nodes/
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
