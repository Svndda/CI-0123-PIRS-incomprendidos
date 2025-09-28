cmake_minimum_required(VERSION 3.19)
project(SafeSpace LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS
  Core Widgets
  LinguistTools
  PrintSupport
  Svg
)

qt_standard_project_setup()

qt_add_executable(SafeSpace
  WIN32 MACOSX_BUNDLE
  src/main.cpp
  
  # Model
  src/model/model.cpp src/model/model.h
  src/model/user.cpp src/model/user.h
  src/model/FIleSystem.cpp src/model/FileSystem.h
  src/model/File.cpp src/model/File.hpp
  src/model/Directory.cpp src/model/Directory.hpp
  src/model/iNode.hpp
  
  # Controller
  src/controller/appcontroller.cpp src/controller/appcontroller.h
  src/controller/UserController.cpp  src/controller/UserController.h
  src/controller/mainwindow.ui
  
  src/view/loginpage.cpp src/view/loginpage.h src/view/loginpage.ui
  src/view/page.h src/view/page.cpp
  src/colors.h
  src/view/elements/button.h src/view/elements/button.cpp
  src/view/elements/sensordatafield.h src/view/elements/sensordatafield.cpp src/view/elements/sensordatafield.ui
  src/model/sensordata.h src/model/sensordata.cpp

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
)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

target_include_directories(SafeSpace
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/src/model/
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
