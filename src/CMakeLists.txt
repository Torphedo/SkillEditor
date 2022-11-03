add_executable(skill_editor
	"main.cpp"
	"memory-editing.cpp"
	"skill_io.cpp"
	"winAPI.cpp"
	"../vendor/crc_32.c"

	"imgui/imgui_backend.cpp"
	"imgui/program-ui.cpp"

	"vendor/imGUI/backends/imgui_impl_glfw.cpp"
	"vendor/imGUI/backends/imgui_impl_vulkan.cpp"
	"vendor/imGUI/backends/imgui_impl_win32.cpp"
	"vendor/imGUI/imgui.cpp"
	"vendor/imGUI/imgui_draw.cpp"
	"vendor/imGUI/imgui_tables.cpp"
	"vendor/imGUI/imgui_widgets.cpp"
)
