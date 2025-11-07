# Add pd_loader_core import library as a library. This allows us to get linked to
# pd_loader_core.dll at runtime without building it from source.
add_library(pd_loader_core SHARED IMPORTED)
set_target_properties(pd_loader_core PROPERTIES IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/vendor/pd_loader_core/libpd_loader_core.dll.a")

# Compile our DLL
add_library(skill_loader SHARED
	src/dll_main.cxx
	src/loader.cxx

	../src/mods.cxx
	../src/text.c
	../src/pool.c
)

target_link_libraries(skill_loader PRIVATE pd_loader_core bobtail nfd)
target_include_directories(skill_loader PRIVATE "${CMAKE_SOURCE_DIR}/vendor/pd_loader_core/include")
