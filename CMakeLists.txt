cmake_minimum_required(VERSION 3.5)
project(cuda-fluid-sim)
set(CMAKE_CXX_STANDARD 17)

# Create an option to switch between a system sdl library and a vendored SDL library
option(MYGAME_VENDORED "Use vendored libraries" OFF)

if(MYGAME_VENDORED)
    # This assumes you have added SDL as a submodule in vendored/SDL
    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
else()
    # 1. Look for a SDL3 package,
    # 2. look for the SDL3-shared component, and
    # 3. fail if the shared component cannot be found.
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
endif()

add_executable(sim
    src/main.cpp
    lib/bitmap.cpp
    lib/renderer.cpp
)

if(WIN32)
    add_custom_command(
        TARGET sim POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:sim>
        VERBATIM
    )
endif()

target_include_directories(sim PUBLIC include)

# Link to the actual SDL3 library.
target_link_libraries(sim PRIVATE SDL3::SDL3)
