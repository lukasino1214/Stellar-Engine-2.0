cmake_minimum_required(VERSION 3.21)
project(all)

include(cmake/warnings.cmake)
include(cmake/static_analysis.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)

find_package(daxa CONFIG REQUIRED)

add_subdirectory(engine)
add_subdirectory(editor)