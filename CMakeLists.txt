cmake_minimum_required(VERSION 3.21)

include(cmake/warnings.cmake)
include(cmake/static_analysis.cmake)

project(cpp_blank_project VERSION 0.1.0)
add_executable(${PROJECT_NAME} "src/main.cpp")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
set_project_warnings(${PROJECT_NAME})