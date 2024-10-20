add_executable(skill_editor
	"main.cpp"
	"skill_io.cpp"
	"winAPI.cpp"
	"text.cpp"
	"../vendor/crc_32.c"
	"memory-editing.cpp"

	"imgui/imgui_backend.cpp"
	"imgui/program-ui.cpp"

	"../vendor/imGUI/backends/imgui_impl_dx9.cpp"
	"../vendor/imGUI/backends/imgui_impl_win32.cpp"
	"../vendor/imGUI/imgui.cpp"
	"../vendor/imGUI/misc/cpp/imgui_stdlib.cpp"
	"../vendor/imGUI/imgui_demo.cpp"
	"../vendor/imGUI/imgui_draw.cpp"
	"../vendor/imGUI/imgui_tables.cpp"
	"../vendor/imGUI/imgui_widgets.cpp"
)
