cmake_minimum_required(VERSION 3.16) # Latest version on apt, and nothing newer is needed.
project(skill_editor
	VERSION 0.4.0
	LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(vendor/rapidyaml)

include_directories(
	"vendor"
	"vendor/imGUI"
	"vendor/rapidyaml"
)

add_library(imgui STATIC
	vendor/imGUI/backends/imgui_impl_dx9.cpp
	vendor/imGUI/backends/imgui_impl_win32.cpp
	vendor/imGUI/imgui.cpp
	vendor/imGUI/misc/cpp/imgui_stdlib.cpp
	vendor/imGUI/imgui_demo.cpp
	vendor/imGUI/imgui_draw.cpp
	vendor/imGUI/imgui_tables.cpp
	vendor/imGUI/imgui_widgets.cpp
)

add_executable(skill_editor
	src/main.cxx
	src/skill_io.cxx
	src/winAPI.cxx
	src/text.cxx
	src/memory_editing.cxx
	src/userlabels.cxx
	src/gui_loop.cxx
	src/skill_editor.cxx
	src/file.c

	vendor/crc_32.c
)

target_link_libraries(skill_editor d3d9 imgui ryml)