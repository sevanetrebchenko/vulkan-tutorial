cmake_minimum_required(VERSION 3.13)
project(vulkan-tutorial)

set(CMAKE_CXX_STANDARD 17)

add_executable(vulkan-tutorial main.cpp)

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)