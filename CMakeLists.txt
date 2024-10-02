cmake_minimum_required(VERSION 3.16) # Latest version on apt, and nothing newer is needed.
project(skill_editor
	VERSION 0.4.0
	LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)

include_directories(
	"vendor"
	"vendor/imGUI"
	"vendor/lua"
)
add_subdirectory(src)
add_subdirectory(vendor)

target_link_libraries(skill_editor lua d3d9)
