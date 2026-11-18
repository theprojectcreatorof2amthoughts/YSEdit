# YSEdit

**YSEdit** is a lightweight terminal text editor for the Shay ecosystem. It is designed for keyboard-first editing of Shay source files while remaining useful for ordinary text files.

## Current features

- Opens and edits a file passed on the command line.
- Saves with **Ctrl+S** and exits with **Ctrl+Q**.
- Warns before quitting with unsaved changes.
- Arrow keys, Home/End, Page Up/Down, Backspace, and Delete.
- Line numbers, filename/status bar, and a modified indicator.
- Initial syntax coloring for Shay keywords, strings, and numbers.
- Uses the terminal's own ANSI palette instead of fixed RGB colors.

## Terminal-native colors

YSEdit deliberately does not choose a theme or paint a solid background:

| Element | Style |
| --- | --- |
| Normal text and background | Terminal defaults |
| Shay keywords | ANSI cyan |
| Strings | ANSI green |
| Numbers | ANSI yellow |
| Shay !# marker | ANSI magenta |
| Line numbers | Dim |
| Status/title bars | Reverse video |

Because these are ANSI styles, the actual hues come from the user's terminal palette (Dracula, Nord, Solarized, and other terminal themes). Set the NO_COLOR environment variable to disable color output. TERM=dumb also disables colors.

## Build

Requires a C++17 compiler and CMake 3.16 or newer.

### Linux

    cmake -S . -B build
    cmake --build build
    ./build/ysedit hello.shay

### Windows (MSYS2 MinGW)

Install CMake and a MinGW-w64 C++ compiler in your MSYS2 environment, then:

    cmake -S . -B build -G "MinGW Makefiles"
    cmake --build build
    ./build/ysedit.exe hello.shay

### Windows (Visual Studio)

    cmake -S . -B build
    cmake --build build --config Release
    .\build\Release\ysedit.exe hello.shay

## Controls

| Key | Action |
| --- | --- |
| Arrow keys | Move cursor |
| Home / End | Start / end of line |
| Page Up / Page Down | Move by a screen |
| Enter | Insert a line break |
| Backspace / Delete | Delete text |
| Ctrl+S | Save |
| Ctrl+Q | Quit (press again to confirm if modified) |

## Project status

YSEdit now has its first functional editor foundation. Syntax highlighting and terminal compatibility are intentionally small and dependency-free for now; the editor can grow into a more complete Shay-focused editing environment without a heavyweight GUI toolkit.

## License

GPL-3.0. See LICENSE.
