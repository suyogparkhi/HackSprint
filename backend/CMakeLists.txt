cmake_minimum_required(VERSION 3.12)
project(DeribitTrader CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH 
    "/opt/homebrew"
    "/opt/homebrew/opt/libwebsockets"
    "/usr/local/opt/libwebsockets"
)

find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)
find_package(Libwebsockets REQUIRED) 
find_package(spdlog REQUIRED)

add_executable(deribit_trader deribit_trader.cpp)

target_include_directories(deribit_trader PRIVATE 
    ${CURL_INCLUDE_DIRS}
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
    ${SPDLOG_INCLUDE_DIRS}
)

target_link_libraries(deribit_trader 
    nlohmann_json::nlohmann_json
    ${CURL_LIBRARIES}
    ${LIBWEBSOCKETS_LIBRARIES}
    spdlog::spdlog
    pthread 
)

target_compile_definitions(deribit_trader PRIVATE 
    CURL_STATICLIB
)