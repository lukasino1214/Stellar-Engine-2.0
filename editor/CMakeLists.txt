cmake_minimum_required(VERSION 3.21)

project(stellar_engine VERSION 0.1.0)
add_executable(${PROJECT_NAME} 
    "main.cpp" 
    "editor.cpp" 
    "panel/asset_browser_panel.cpp" 
    "panel/scene_hiearchy_panel.cpp" 
    "panel/toolbar_panel.cpp" 
    "panel/performance_stats_panel.cpp"
    "panel/logger_panel.cpp"
    "panel/viewport_panel.cpp"
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
set_project_warnings(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE daxa::daxa)
#target_link_libraries(${PROJECT_NAME} PRIVATE imguizmo::imguizmo)
target_link_libraries(${PROJECT_NAME} PRIVATE Engine)