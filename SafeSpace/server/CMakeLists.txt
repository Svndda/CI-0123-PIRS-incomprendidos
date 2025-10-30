cmake_minimum_required(VERSION 4.0)
project(server)

set(CMAKE_CXX_STANDARD 14)

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
        src/nodes/CriticalEvents/CriticalEventsNode.cpp
        src/nodes/CriticalEvents/CriticalEventsNode.h
        src/main.cpp
        src/nodes/interfaces/UDPServer.cpp
        src/nodes/interfaces/UDPServer.h
        src/model/structures/DiscoverRequest.h
        src/model/structures/DiscoverResponse.h
)
