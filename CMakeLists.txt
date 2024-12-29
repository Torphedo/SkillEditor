cmake_minimum_required(VERSION 3.16) # Latest version on apt, and nothing newer is needed.
project(skill_editor
	VERSION 0.4.0
	LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(vendor/rapidyaml)
add_subdirectory(vendor/bobtail)

include_directories(
	"vendor"
	"vendor/imgui"
	"vendor/rapidyaml"
	"vendor/bobtail"
)

add_library(imgui STATIC
	vendor/imgui/backends/imgui_impl_dx9.cpp
	vendor/imgui/backends/imgui_impl_win32.cpp
	vendor/imgui/imgui.cpp
	vendor/imgui/misc/cpp/imgui_stdlib.cpp
	vendor/imgui/imgui_demo.cpp
	vendor/imgui/imgui_draw.cpp
	vendor/imgui/imgui_tables.cpp
	vendor/imgui/imgui_widgets.cpp
)

add_executable(skill_editor
	src/main.cxx
	src/mods.cxx
	src/winAPI.cxx
	src/text.cxx
	src/remote_pd.cxx
	src/userlabels.cxx
	src/gui_loop.cxx
	src/skill_editor.cxx
	src/pool.c
)

target_link_libraries(skill_editor d3d9 imgui ryml bobtail)