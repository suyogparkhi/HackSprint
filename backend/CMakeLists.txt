cmake_minimum_required(VERSION 3.12)
project(DeribitTrader CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH 
    "/opt/homebrew"
    "/opt/homebrew/opt/libwebsockets"
    "/usr/local/opt/libwebsockets"
)

# Find required packages
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)
find_package(Libwebsockets REQUIRED) 
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)

# Add source files
set(SOURCES
    src/deribit_trader.cpp
    src/trading_agent.cpp
    src/main.cpp
)

# Add executable
add_executable(deribit_trader ${SOURCES})

# Include directories
target_include_directories(deribit_trader PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIRS}
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
    ${SPDLOG_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(deribit_trader PRIVATE
    nlohmann_json::nlohmann_json
    ${CURL_LIBRARIES}
    ${LIBWEBSOCKETS_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    spdlog::spdlog
    pthread 
)

# Compile definitions
target_compile_definitions(deribit_trader PRIVATE 
    CURL_STATICLIB
)