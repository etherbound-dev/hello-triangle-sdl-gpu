cmake_minimum_required(VERSION 3.22)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
project(Example)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources)
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})

add_executable(Example main.c)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
target_link_libraries(Example PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)
