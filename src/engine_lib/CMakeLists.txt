project(engine_lib CXX)

set(PROJECT_SOURCES
    include/game_instance.h
    include/window.h
    include/world/world_manager.h
    include/world/world.h
    include/render/render_target.h
    include/io/log.h
    include/io/paths.h
    include/io/filesystem.h
    include/misc/globals.h
    include/misc/string_funcs.h
    include/node/node.h
    include/node/spatial_node.h
    src/game_instance.cpp
    src/window.cpp
    src/world/world_manager.cpp
    src/world/world.cpp
    src/render/render_target.cpp
    src/io/log.cpp
    src/io/paths.cpp
    src/io/filesystem.cpp
    src/misc/globals.cpp
    src/misc/string_funcs.cpp
    src/node/node.cpp
    src/node/spatial_node.cpp)

# Define targets (one without any specific macros and another for compilation with editor).
set(ENGINE_LIB_TARGET engine_lib)
set(ENGINE_LIB_TARGET_FOR_EDITOR engine_lib_for_editor)
add_library(${ENGINE_LIB_TARGET} STATIC ${PROJECT_SOURCES})
add_library(${ENGINE_LIB_TARGET_FOR_EDITOR} STATIC ${PROJECT_SOURCES})

# -------------------------------------------------------------------------------------------------
#                                         CONFIGURE TARGET
# -------------------------------------------------------------------------------------------------

glow_engine_configure_target(${ENGINE_LIB_TARGET})
glow_engine_configure_target(${ENGINE_LIB_TARGET_FOR_EDITOR})

# Define editor macros for editor's version of the engine_lib.
target_compile_definitions(${ENGINE_LIB_TARGET_FOR_EDITOR} PUBLIC ENGINE_EDITOR)

if (IS_RELEASE_BUILD)
    # The editor needs debug drawing and debug console even in release builds.
    target_compile_definitions(${ENGINE_LIB_TARGET_FOR_EDITOR} PUBLIC ENGINE_DEBUG_TOOLS)
endif()

# -------------------------------------------------------------------------------------------------
#                                       DEPENDENCIES
# -------------------------------------------------------------------------------------------------

# Group all dependencies into a single interface library, later we will link it to 2 version of the engine_lib
# one for the editor (with some editor macros) and one for the game.
set(ENGINE_LIB_DEPS_TARGET engine_lib_deps)
add_library(${ENGINE_LIB_DEPS_TARGET} INTERFACE)

# Add SDL.
message(STATUS "${PROJECT_NAME}: adding dependency \"SDL\"...")
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_TESTS OFF CACHE BOOL "" FORCE)
target_include_directories(${ENGINE_LIB_DEPS_TARGET} SYSTEM INTERFACE ${RELATIVE_EXT_PATH}/SDL/include)
add_subdirectory(${RELATIVE_EXT_PATH}/SDL ${DEPENDENCY_BUILD_DIR_NAME}/SDL EXCLUDE_FROM_ALL)
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE SDL3::SDL3)

# Win32 specific dependencies:
if (WIN32)
    # Add D3D12MemoryAllocator.
    message(STATUS "${PROJECT_NAME}: adding dependency \"D3D12MemoryAllocator\"...")
    add_subdirectory(${RELATIVE_EXT_PATH}/D3D12MemoryAllocator ${DEPENDENCY_BUILD_DIR_NAME}/D3D12MemoryAllocator SYSTEM)
    target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE D3D12MemoryAllocator)
    if(NOT MSVC)
        set_source_files_properties(${RELATIVE_EXT_PATH}/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp PROPERTIES COMPILE_FLAGS "-Wno-tautological-undefined-compare")
        set_source_files_properties(${RELATIVE_EXT_PATH}/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp PROPERTIES COMPILE_FLAGS "-Wno-switch")
    endif()

    # Add DirectX-Headers.
    message(STATUS "${PROJECT_NAME}: adding dependency \"DirectX-Headers\"...")
    add_subdirectory(${RELATIVE_EXT_PATH}/DirectX-Headers ${DEPENDENCY_BUILD_DIR_NAME}/DirectX-Headers SYSTEM)
    target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE DirectX-Headers)
    if(NOT MSVC)
        set_source_files_properties(${RELATIVE_EXT_PATH}/DirectX-Headers/src/d3dx12_property_format_table.cpp PROPERTIES COMPILE_FLAGS "-Wno-switch")
    endif()
endif()

# Link engine library to dependencies.
target_link_libraries(${ENGINE_LIB_TARGET} PUBLIC ${ENGINE_LIB_DEPS_TARGET})
target_link_libraries(${ENGINE_LIB_TARGET_FOR_EDITOR} PUBLIC ${ENGINE_LIB_DEPS_TARGET})
