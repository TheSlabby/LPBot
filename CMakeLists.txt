cmake_minimum_required(VERSION 3.20)
project(LPBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(dpp REQUIRED)


add_executable(lpbot src/main.cpp)

target_sources(lpbot PRIVATE
    src/LPBot.cpp
)

target_link_libraries(lpbot PRIVATE
    dpp::dpp
)