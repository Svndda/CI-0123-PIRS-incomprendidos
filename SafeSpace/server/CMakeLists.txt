cmake_minimum_required(VERSION 3.10)
project(server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

include_directories(src)
include_directories(src/model)
include_directories(src/model/filesystem)
include_directories(src/model/managers)
include_directories(src/model/structures)
include_directories(src/nodes)

add_executable(server
        src/model/filesystem/Directory.cpp
        src/model/filesystem/Directory.hpp
        src/model/filesystem/DirEntry.h
        src/model/filesystem/DiskManager.cpp
        src/model/filesystem/DiskManager.h
        src/model/filesystem/FileSystem.cpp
        src/model/filesystem/FileSystem.h
        src/model/filesystem/iNode.h
        src/model/filesystem/Layout.h
        src/model/managers/UsersManager.cpp
        src/model/managers/UsersManager.h
        src/model/structures/connectrequest.cpp
        src/model/structures/connectrequest.h
        src/model/structures/sensordata.cpp
        src/model/structures/sensordata.h
        src/model/structures/user.cpp
        src/model/structures/user.h
        src/model/model.cpp
        src/model/model.h
        src/nodes/SafeSpaceServer.cpp
        src/nodes/SafeSpaceServer.h
        src/nodes/Intermediary/IntermediaryNode.cpp
        src/nodes/Intermediary/IntermediaryNode.h
        src/nodes/Arduino/Arduino_Node.cpp
        src/nodes/Arduino/Arduino_Node.h
        src/nodes/CriticalEvents/CriticalEventsNode.cpp
        src/nodes/CriticalEvents/CriticalEventsNode.h
        src/nodes/Proxy/ProxyNode.cpp
        src/nodes/Proxy/ProxyNode.h
        src/nodes/Auth/auth_udp_server.cpp
        src/nodes/Auth/auth_udp_server.h
        ../common/LogManager.cpp
        ../common/LogManager.h
        src/main.cpp
        src/nodes/interfaces/UDPServer.cpp
        src/nodes/interfaces/UDPServer.h
        src/nodes/interfaces/UDPClient.cpp
        src/nodes/interfaces/UDPClient.h
        src/model/structures/DiscoverRequest.h
        src/model/structures/DiscoverResponse.h
        src/model/structures/authenticationrequest.h
        src/model/structures/authenticationresponse.h
        src/model/structures/SensorPacket.h
        src/nodes/Storage/StorageNode.h
        src/nodes/Storage/StorageNode.cpp
        src/nodes/Proxy/ProxyNode.h
        src/nodes/Proxy/ProxyNode.cpp
        src/nodes/Bootstrap/Bootstrap.cpp
        src/nodes/Bootstrap/Bootstrap.h
)

# Link libraries
target_link_libraries(server 
        OpenSSL::SSL OpenSSL::Crypto
        Qt6::Core Qt6::Gui Qt6::Widgets
        OpenSSL::SSL OpenSSL::Crypto
)
