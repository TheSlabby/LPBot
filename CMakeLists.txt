cmake_minimum_required(VERSION 3.28)
project(LPBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(dpp REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(zstd REQUIRED)
find_package(httplib REQUIRED)

# httplib looking for another name
if(NOT TARGET zstd::libzstd)
    add_library(zstd::libzstd INTERFACE IMPORTED)
    target_link_libraries(zstd::libzstd INTERFACE ${ZSTD_LIBRARIES})
    target_include_directories(zstd::libzstd INTERFACE ${ZSTD_INCLUDE_DIRS})
endif()

# fetch json
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_executable(lpbot src/main.cpp)

target_sources(lpbot PRIVATE
    src/LPBot.cpp
    src/MatchDB.cpp
    src/RiotAPI.cpp
)

target_link_libraries(lpbot PRIVATE
    dpp::dpp
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
    httplib::httplib
    zstd
)