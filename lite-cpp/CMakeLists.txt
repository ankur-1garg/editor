# Minimum CMake version required
cmake_minimum_required(VERSION 3.15)

# Project definition
project(lite_cpp VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard (C++20 required)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF) # Optional: Disable vendor extensions

# Set MSVC specific flags
if(MSVC)
    # Force MSVC to use C++20 features and UTF-8
    add_compile_options(/std:c++20 /utf-8)
    
    # Set runtime library to static for PDCurses
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    # Add Windows-specific definitions
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()

# --- Find Dependencies ---

# Find PDCurses on Windows
if(WIN32)
    find_package(unofficial-pdcurses CONFIG REQUIRED)
    set(CURSES_LIBRARIES unofficial::pdcurses::pdcurses)
    # Add PDCurses include directories - use absolute path to vcpkg includes
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/packages/pdcurses_x64-windows/include)
else()
    # Keep existing ncurses finding for non-Windows platforms
    find_package(Curses REQUIRED)
    if(NOT CURSES_FOUND)
        message(FATAL_ERROR "Could not find Curses library (ncurses). Please install it.")
    endif()
    include_directories(${CURSES_INCLUDE_DIR})
endif()

# --- Define Executable Target ---

# Add the main executable target
# We'll add more source files here as we create them
add_executable(lite_cpp
    src/main.cpp
    # Add other .cpp files here later, e.g.:
    src/Editor.cpp
    src/Buffer.cpp
    src/Change.cpp
    src/frontend/Terminal.cpp
    src/lang/Expr.cpp
    src/lang/Environment.cpp
    src/lang/Parser.cpp
    src/lang/Interpreter.cpp
    src/lang/Builtin.cpp
)

# --- Link Libraries ---

# Link the executable against required libraries
if(WIN32)
    target_link_libraries(lite_cpp PRIVATE unofficial::pdcurses::pdcurses)
else()
    target_link_libraries(lite_cpp PRIVATE ${CURSES_LIBRARIES})
endif()

# --- Optional: Installation ---
# Add installation rules if desired later
# install(TARGETS lite_cpp DESTINATION bin)

# --- Optional: Testing ---
# enable_testing()
# Add tests later if desired

# --- Output ---
message(STATUS "Configuring lite-cpp editor...")
message(STATUS "Using Curses library: ${CURSES_LIBRARIES}")