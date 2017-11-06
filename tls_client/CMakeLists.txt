cmake_minimum_required(VERSION 3.8)
project(tls_raw_client)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c)
add_executable(tls_client ${SOURCE_FILES})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(tls_client ${OPENSSL_LIBRARIES})

if (WIN32)
    target_link_libraries(tls_client ws2_32)
    target_link_libraries(tls_client ws2_32)
endif()