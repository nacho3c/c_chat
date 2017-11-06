cmake_minimum_required(VERSION 3.0)
project(cchat)

option(BUILD_CLIENT "Build the client executable" ON)
option(BUILD_SERVER "Build the server executable" ON)

set(MAX_USER_LEN 21)
set(MAX_CHAT_USERS 30)

set(CMAKE_C_STANDARD 99)

set(CCHAT_VERSION_MAJOR 1)
set(CCHAT_VERSION_MINOR 0)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

configure_file(config.tpl.h config.h)

#set(COMMON_FILES log.c chat.c log.h chat.h)

add_executable(server server.c ${COMMON_FILES})
#add_executable(client client.c ${COMMON_FILES})

if (WIN32)
    target_link_libraries(server wsock32 ws2_32)
    #target_link_libraries(client wsock32 ws2_32)
endif ()