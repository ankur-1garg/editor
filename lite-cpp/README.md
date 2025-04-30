# lite-cpp 📝

*(Inspired by adam-mcdaniel/lite)*

`lite-cpp` is a lightweight and extensible terminal text editor implemented in C++, aiming to replicate the core concepts of the original Rust `lite` editor. It features a minimal core with a custom scripting language for extensibility.

This project serves as an exercise in C++ development, terminal UI programming, and potentially language implementation.

## Table of Contents

- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
    - [Keybindings](#keybindings)
    - [Scripting](#scripting)
- [Contributing](#contributing)
- [License](#license)

## Features

*(Features based on the original `lite` editor - implementation status may vary)*

- [ ] **Basic editing operations**: cut, copy, paste, find, etc.
- [ ] **Syntax highlighting**: Simple rule-based highlighting.
- [ ] **Undo/redo**: Handling of undo/redo operations.
- [ ] **Scripting Support**: Builtin Lisp-like scripting language.
- [ ] **Shell commands**: Run shell commands from within the editor.
- [ ] **Line numbers and status bar**: Standard editor UI elements.
- [ ] **Multiple buffers**: Support for multiple files/scratch buffers.
- [ ] **Save and open files**: Basic file I/O.
- [ ] **Simple keybindings**: Easier to learn than Vim/Emacs.

## Installation

You will need a C++17 compliant compiler (like GCC or Clang) and the CMake build system. You also need the `ncurses` development library (e.g., `libncurses-dev` on Debian/Ubuntu, `ncurses-devel` on Fedora, or `pdcurses` might be needed on Windows).

1.  **Clone the repository:**
    ```bash
    git clone <repository-url>
    cd lite-cpp
    ```
2.  **Configure using CMake:**
    ```bash
    cmake -S . -B build
    ```
3.  **Build the project:**
    ```bash
    cmake --build build
    ```
4.  **Run:** The executable `lite_cpp` will be in the `build` directory.
    ```bash
    ./build/lite_cpp <filename>
    ```

## Usage

To open a file in `lite-cpp`, run:

```bash
./build/lite_cpp <filename>