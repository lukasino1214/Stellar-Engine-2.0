cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Engine)

add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
    "core/logger.cpp" 
    "core/types.cpp" 
    "core/uuid.cpp" 
    "core/window.cpp" 
    "data/scene.cpp" 
    "data/entity.hpp"
    "data/entity.cpp"
    "data/components.cpp"
    "graphics/texture.cpp"
    "graphics/camera.cpp"
    "graphics/model.cpp"
    "utils/utils.hpp"
    "utils/utils.cpp"
    "utils/gui.hpp"
    "utils/gui.cpp"
)

set_project_warnings(${PROJECT_NAME})

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

find_package(OpenAL CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenAL::OpenAL)

target_link_libraries(${PROJECT_NAME} PRIVATE daxa::daxa)

find_package(EnTT CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE EnTT::EnTT)

find_package(yaml-cpp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt-header-only)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    EXPORT ${PROJECT_NAME}-config
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
    DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)