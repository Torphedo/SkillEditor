cmake_minimum_required(VERSION 3.16) # Latest version on apt, and nothing newer is needed.
project(skill_editor
	VERSION 0.2.0
	LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

# Disable extra GLFW modules
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("vendor/GLFW")

include_directories(
	"vendor"
	"vendor/GLFW/include"
	"vendor/imGUI"
	"${Vulkan_INCLUDE_DIRS}"
)
add_subdirectory(src)

target_link_libraries(skill_editor glfw ${Vulkan_LIBRARIES})
