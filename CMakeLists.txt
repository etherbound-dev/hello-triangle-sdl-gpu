cmake_minimum_required(VERSION 3.22)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
project(Example)

file(GLOB files "${CMAKE_SOURCE_DIR}/shaders/source/*.hlsl")
set(SHADERCROSS $ENV{SHADERCROSS_DIR}/shadercross.exe)
foreach(file ${files})
  get_filename_component(name ${file} NAME_WLE)
  message(STATUS "Compiling shader ${name}")
  execute_process(
    COMMAND ${SHADERCROSS} "${CMAKE_SOURCE_DIR}/shaders/source/${name}.hlsl" -d
            DXIL --output ${CMAKE_SOURCE_DIR}/shaders/compiled/${name}.dxil
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
    RESULT_VARIABLE result COMMAND_ECHO STDOUT)
  message(STATUS "Output: ${output}")
  if(result)
    message(FATAL_ERROR "Shader compilation failed: ${error}")
  else()
    message(STATUS "Shader compiled successfully: ${name}")
  endif()
endforeach()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources)
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})

file(GLOB compiled_shaders "${CMAKE_SOURCE_DIR}/shaders/compiled/*")
foreach(file ${compiled_shaders})
  get_filename_component(name ${file} NAME_WLE)
  message(STATUS "Copying shader ${name}")
  file(COPY ${file} DESTINATION ${CMAKE_BINARY_DIR}/shaders)
endforeach()

add_executable(Example main.c)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
target_link_libraries(Example PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)
