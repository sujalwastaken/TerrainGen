cmake_minimum_required(VERSION 3.20)
project(TerrainGen CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

find_path(STB_INCLUDE_DIR "stb_image.h")

add_executable(TerrainGen 
    src/main.cpp 
    src/TerrainGenerator.cpp 
    src/TerrainRenderer.cpp
)

target_include_directories(TerrainGen PRIVATE 
    ${STB_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(TerrainGen PRIVATE
    glfw
    glad::glad
    imgui::imgui
    implot::implot
    glm::glm
)