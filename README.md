# YSEdit

**YSEdit** is a lightweight terminal text editor for the Shay ecosystem. It is designed for keyboard-first editing of Shay source files while remaining useful for ordinary text files.

## Current features

- Opens and edits a file passed on the command line.
- Saves with **Ctrl+S** and exits with **Ctrl+X** (Ctrl+Q remains an alias).
- Prompts to save, discard, or cancel when exiting with unsaved changes.
- Fetches Shay's machine-readable keyword list from the Shay repository at startup, so new language keywords can be highlighted without editing YSEdit.
- Arrow keys, Home/End, Page Up/Down, Backspace, and Delete.
- Tab characters are preserved and displayed at 8-column tab stops; horizontal scrolling and cursor placement account for their visual width.
- Interactive search with **Ctrl+F**, then **Ctrl+N** / **Ctrl+P** to jump between matches.
- Line numbers, filename/status bar, and a modified indicator.
- Restores the shell's previous terminal screen and cursor on exit, like nano.
- Syntax coloring for Shay keywords, strings, and numbers. Keyword highlighting uses the repository's `syntax/keywords.txt`; a built-in keyword set is used when fetching is unavailable.
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

Requires a C++17 compiler. You can build with GNU Make or CMake.

### GNU Make (Linux, macOS, or MSYS2)

    make
    ./build/ysedit hello.shay

Useful targets:

    make debug
    make release
    make run FILE=hello.shay
    make clean
    make install

Override the compiler with `make CXX=clang++`, or customize `CXXFLAGS`.

### CMake (cross-platform)

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
| Tab | Insert a tab character |
| Backspace / Delete | Delete text |
| Ctrl+F | Search |\n| Ctrl+N / Ctrl+P | Next / previous search match |\n| Ctrl+S | Save |
| Ctrl+X | Quit (prompts to save, discard, or cancel if modified) |
| Ctrl+Q | Legacy quit shortcut (same behavior as Ctrl+X) |

## Project status

YSEdit now has its first functional editor foundation. Syntax highlighting stays lightweight: YSEdit optionally uses `curl` to fetch the current Shay keyword manifest at startup and falls back to its built-in vocabulary when offline or when `curl` is unavailable.

## License

GPL-3.0. See LICENSE.
