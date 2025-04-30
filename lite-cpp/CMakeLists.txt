# Minimum CMake version required
cmake_minimum_required(VERSION 3.15)

# Project definition
project(lite_cpp VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard (C++17 required for optional, variant, filesystem)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF) # Optional: Disable vendor extensions

# --- Find Dependencies ---

# Find the ncurses (or pdcurses on Windows) library
# You might need to install ncurses-dev (Linux) or pdcurses (Windows)
find_package(Curses REQUIRED)
if(NOT CURSES_FOUND)
    message(FATAL_ERROR "Could not find Curses library (ncurses/pdcurses). Please install it.")
endif()

# Add include directories for dependencies
include_directories(${CURSES_INCLUDE_DIR})

# --- Define Executable Target ---

# Add the main executable target
# We'll add more source files here as we create them
add_executable(lite_cpp
    src/main.cpp
    # Add other .cpp files here later, e.g.:
    # src/Editor.cpp
    # src/Buffer.cpp
    # src/Change.cpp
    # src/frontend/Terminal.cpp
    # src/lang/Expr.cpp
    # src/lang/Environment.cpp
    # src/lang/Parser.cpp
    # src/lang/Interpreter.cpp
    # src/lang/Builtin.cpp
)

# --- Link Libraries ---

# Link the executable against required libraries
target_link_libraries(lite_cpp PRIVATE ${CURSES_LIBRARIES})

# --- Optional: Installation ---
# Add installation rules if desired later
# install(TARGETS lite_cpp DESTINATION bin)

# --- Optional: Testing ---
# enable_testing()
# Add tests later if desired

# --- Output ---
message(STATUS "Configuring lite-cpp editor...")
message(STATUS "Using Curses library: ${CURSES_LIBRARIES}")