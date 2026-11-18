#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {
struct Key {
    enum Code { Character, Enter, Backspace, Delete, Left, Right, Up, Down, Home, End, PageUp, PageDown, Quit, Save, Find, NextMatch, PreviousMatch, Unknown } code = Unknown;
    char ch = 0;
};

class Terminal {
public:
    Terminal() {
#ifdef _WIN32
        output_ = GetStdHandle(STD_OUTPUT_HANDLE);
        // Native Windows consoles expose a console handle. MSYS2 terminals
        // such as mintty/CLANG64 often expose a tty without one, so detect
        // both. The alternate-screen escape sequences work in either tty.
        screenActive_ = _isatty(_fileno(stdout)) != 0;
        if (GetConsoleMode(output_, &originalOutputMode_)) {
            haveOutputMode_ = true;
            if (SetConsoleMode(output_, originalOutputMode_ |
                    ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT))
                screenActive_ = true;
        }
#else
        screenActive_ = isatty(STDOUT_FILENO) != 0;
        if (isatty(STDIN_FILENO)) {
            tcgetattr(STDIN_FILENO, &original_);
            termios raw = original_;
            raw.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON | IEXTEN | ISIG));
            raw.c_iflag &= static_cast<tcflag_t>(~(IXON | ICRNL | BRKINT | INPCK | ISTRIP));
            raw.c_oflag &= static_cast<tcflag_t>(~OPOST);
            raw.c_cflag |= CS8;
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
            active_ = true;
        }
#endif
        const char* noColor = std::getenv("NO_COLOR");
        const char* term = std::getenv("TERM");
        color_ = noColor == nullptr && !(term && std::string(term) == "dumb");
        if (screenActive_)
            std::cout << "\x1b[?1049h\x1b[?25l\x1b[2J" << std::flush;
    }

    ~Terminal() {
        restoreScreen();
#ifndef _WIN32
        if (active_) tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_);
#else
        if (haveOutputMode_) SetConsoleMode(output_, originalOutputMode_);
#endif
    }

    // Leave the terminal exactly as the user found it, like nano.
    // The destructor calls this too, so restoration is a safe fallback.
    void restoreScreen() {
        if (!screenActive_) return;
        std::cout << "\x1b[0m\x1b[?25h\x1b[?1049l" << std::flush;
        screenActive_ = false;
    }

    void move(int row, int col) { std::cout << "\x1b[" << row << ';' << col << 'H'; }
    void reset() { if (color_) std::cout << "\x1b[0m"; }
    void dim() { if (color_) std::cout << "\x1b[2m"; }
    void cyan() { if (color_) std::cout << "\x1b[36m"; }
    void green() { if (color_) std::cout << "\x1b[32m"; }
    void yellow() { if (color_) std::cout << "\x1b[33m"; }
    void magenta() { if (color_) std::cout << "\x1b[35m"; }
    void reverse() { if (color_) std::cout << "\x1b[7m"; }
    void hideCursor() { std::cout << "\x1b[?25l"; }
    void showCursor() { std::cout << "\x1b[?25h" << std::flush; }

    std::pair<int, int> size() const {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (GetConsoleScreenBufferInfo(output_, &info))
            return {info.srWindow.Bottom - info.srWindow.Top + 1, info.srWindow.Right - info.srWindow.Left + 1};
#else
        winsize ws{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row && ws.ws_col)
            return {ws.ws_row, ws.ws_col};
#endif
        return {24, 80};
    }

    Key readKey() {
#ifdef _WIN32
        int c = _getch();
        if (c == 0 || c == 224) {
            switch (_getch()) {
                case 72: return {Key::Up}; case 80: return {Key::Down};
                case 75: return {Key::Left}; case 77: return {Key::Right};
                case 71: return {Key::Home}; case 79: return {Key::End};
                case 73: return {Key::PageUp}; case 81: return {Key::PageDown};
                case 83: return {Key::Delete}; default: return {};
            }
        }
#else
        unsigned char c = 0;
        if (read(STDIN_FILENO, &c, 1) != 1) return {};
        if (c == 27) {
            unsigned char a = 0, b = 0;
            if (read(STDIN_FILENO, &a, 1) != 1 || a != '[' || read(STDIN_FILENO, &b, 1) != 1) return {};
            if (b >= '0' && b <= '9') {
                unsigned char tail = 0;
                if (read(STDIN_FILENO, &tail, 1) != 1 || tail != '~') return {};
                if (b == '3') return {Key::Delete};
                if (b == '5') return {Key::PageUp};
                if (b == '6') return {Key::PageDown};
                return {};
            }
            switch (b) {
                case 'A': return {Key::Up}; case 'B': return {Key::Down};
                case 'C': return {Key::Right}; case 'D': return {Key::Left};
                case 'H': return {Key::Home}; case 'F': return {Key::End};
                default: return {};
            }
        }
#endif
        if (c == 9) return {Key::Character, '\t'};
        if (c == 17) return {Key::Quit};
        if (c == 19) return {Key::Save};
        if (c == 6) return {Key::Find};
        if (c == 14) return {Key::NextMatch};
        if (c == 16) return {Key::PreviousMatch};
        if (c == 13 || c == 10) return {Key::Enter};
        if (c == 8 || c == 127) return {Key::Backspace};
        if (c >= 32 && c <= 126) return {Key::Character, static_cast<char>(c)};
        return {};
    }

private:
    bool color_ = true;
    bool screenActive_ = false;
#ifdef _WIN32
    HANDLE output_ = INVALID_HANDLE_VALUE;
    DWORD originalOutputMode_ = 0;
    bool haveOutputMode_ = false;
#else
    termios original_{};
    bool active_ = false;
#endif
};

class Editor {
public:
    explicit Editor(std::string filename) : filename_(std::move(filename)) {
        if (filename_.empty()) filename_ = "[No Name]";
        else load();
        if (lines_.empty()) lines_.emplace_back();
    }
    void run() {
        while (!quit_) {
            render();
            process(terminal_.readKey());
        }
        terminal_.restoreScreen();
    }

private:
    Terminal terminal_;
    std::string filename_;
    std::vector<std::string> lines_;
    int row_ = 0, col_ = 0, rowOffset_ = 0, colOffset_ = 0;
    bool dirty_ = false, quit_ = false, quitArmed_ = false;
    std::string message_ = "Ctrl-S save | Ctrl-Q quit | Arrows move";

    void load() {
        std::ifstream in(filename_, std::ios::binary);
        if (!in) { message_ = "New file: " + filename_; return; }
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines_.push_back(line);
        }
        message_ = "Opened " + filename_;
    }

    bool save() {
        if (filename_ == "[No Name]") {
            message_ = "Start YSEdit with a filename to save";
            return false;
        }
        std::ofstream out(filename_, std::ios::binary | std::ios::trunc);
        if (!out) { message_ = "Could not write " + filename_; return false; }
        for (std::size_t i = 0; i < lines_.size(); ++i) {
            out << lines_[i];
            if (i + 1 < lines_.size()) out << '\n';
        }
        if (!out) { message_ = "Write failed"; return false; }
        dirty_ = false;
        message_ = "Saved " + filename_;
        return true;
    }

    int visibleRows() const { return std::max(1, terminal_.size().first - 2); }

    static int visualColumn(const std::string& line, int byteColumn) {
        int visual = 0;
        const int limit = std::min(byteColumn, static_cast<int>(line.size()));
        for (int i = 0; i < limit; ++i) {
            if (line[i] == '\t') visual += 8 - (visual % 8);
            else ++visual;
        }
        return visual;
    }

    static int byteColumnAtVisual(const std::string& line, int targetVisual) {
        int visual = 0;
        for (int i = 0; i < static_cast<int>(line.size()); ++i) {
            if (visual >= targetVisual) return i;
            const int next = visual + (line[i] == '\t' ? 8 - (visual % 8) : 1);
            if (targetVisual < next) return i;
            visual = next;
        }
        return static_cast<int>(line.size());
    }

    void process(Key key) {
        if (key.code != Key::Quit) quitArmed_ = false;
        switch (key.code) {
            case Key::Quit:
                if (!dirty_) quit_ = true;
                else if (quitArmed_) quit_ = true;
                else { quitArmed_ = true; message_ = "Unsaved changes! Ctrl-Q again to discard, or Ctrl-S to save."; }
                break;
            case Key::Save: save(); break;
            case Key::Find: {
                std::string query = prompt("Find: ");
                if (!query.empty()) findMatch(query, true);
                break;
            }
            case Key::NextMatch: findMatch(lastSearch_, true); break;
            case Key::PreviousMatch: findMatch(lastSearch_, false); break;
            case Key::Character:
                lines_[row_].insert(lines_[row_].begin() + col_, key.ch); ++col_; dirty_ = true; break;
            case Key::Enter: {
                std::string tail = lines_[row_].substr(col_);
                lines_[row_].erase(col_);
                lines_.insert(lines_.begin() + row_ + 1, tail);
                ++row_; col_ = 0; dirty_ = true; break;
            }
            case Key::Backspace:
                if (col_ > 0) { lines_[row_].erase(col_ - 1, 1); --col_; dirty_ = true; }
                else if (row_ > 0) {
                    col_ = static_cast<int>(lines_[row_ - 1].size());
                    lines_[row_ - 1] += lines_[row_];
                    lines_.erase(lines_.begin() + row_); --row_; dirty_ = true;
                }
                break;
            case Key::Delete:
                if (col_ < static_cast<int>(lines_[row_].size())) { lines_[row_].erase(col_, 1); dirty_ = true; }
                else if (row_ + 1 < static_cast<int>(lines_.size())) {
                    lines_[row_] += lines_[row_ + 1];
                    lines_.erase(lines_.begin() + row_ + 1); dirty_ = true;
                }
                break;
            case Key::Left:
                if (col_ > 0) --col_;
                else if (row_ > 0) { --row_; col_ = static_cast<int>(lines_[row_].size()); }
                break;
            case Key::Right:
                if (col_ < static_cast<int>(lines_[row_].size())) ++col_;
                else if (row_ + 1 < static_cast<int>(lines_.size())) { ++row_; col_ = 0; }
                break;
            case Key::Up:
                if (row_ > 0) {
                    const int target = visualColumn(lines_[row_], col_);
                    --row_;
                    col_ = byteColumnAtVisual(lines_[row_], target);
                }
                break;
            case Key::Down:
                if (row_ + 1 < static_cast<int>(lines_.size())) {
                    const int target = visualColumn(lines_[row_], col_);
                    ++row_;
                    col_ = byteColumnAtVisual(lines_[row_], target);
                }
                break;
            case Key::Home: col_ = 0; break;
            case Key::End: col_ = static_cast<int>(lines_[row_].size()); break;
            case Key::PageUp: row_ = std::max(0, row_ - visibleRows()); col_ = std::min(col_, static_cast<int>(lines_[row_].size())); break;
            case Key::PageDown: row_ = std::min(static_cast<int>(lines_.size()) - 1, row_ + visibleRows()); col_ = std::min(col_, static_cast<int>(lines_[row_].size())); break;
            default: break;
        }
    }

    std::string lastSearch_;

    std::string prompt(const std::string& label) {
        std::string value;
        message_ = label;
        while (true) {
            render();
            Key key = terminal_.readKey();
            if (key.code == Key::Enter) break;
            if (key.code == Key::Backspace) {
                if (!value.empty()) value.pop_back();
            } else if (key.code == Key::Character) {
                value.push_back(key.ch);
            } else if (key.code == Key::Quit) {
                message_ = "Search cancelled";
                return {};
            }
            message_ = label + value;
        }
        return value;
    }

    void findMatch(const std::string& query, bool forward) {
        if (query.empty()) {
            message_ = "No previous search";
            return;
        }
        lastSearch_ = query;
        const int count = static_cast<int>(lines_.size());
        for (int step = 0; step < count; ++step) {
            int targetRow = forward ? (row_ + step) % count : (row_ - step + count) % count;
            const std::string& line = lines_[targetRow];
            std::size_t found = std::string::npos;
            if (forward) {
                std::size_t start = step == 0 ? static_cast<std::size_t>(std::min(col_ + 1, static_cast<int>(line.size()))) : 0;
                found = line.find(query, start);
            } else {
                std::size_t start = step == 0 ? static_cast<std::size_t>(std::max(0, col_ - 1)) : line.size();
                found = line.rfind(query, start);
            }
            if (found != std::string::npos) {
                row_ = targetRow;
                col_ = static_cast<int>(found);
                message_ = "Found: " + query;
                return;
            }
        }
        message_ = "Not found: " + query;
    }

    static bool wordChar(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c == '!';
    }

    void drawSyntax(const std::string& line, int startColumn, int width) {
        const bool shay = filename_.size() >= 5 &&
            (filename_.substr(filename_.size() - 5) == ".shay" || filename_.substr(filename_.size() - 5) == "shaii");
        int visual = 0;
        const int endColumn = startColumn + width;

        // Tabs remain single bytes in the file, but occupy cells up to the
        // next 8-column tab stop on screen. Clip by visual columns so
        // horizontal scrolling and cursor placement use the same coordinates.
        auto emit = [&](char c) {
            const int cells = c == '\t' ? 8 - (visual % 8) : 1;
            for (int cell = 0; cell < cells; ++cell) {
                if (visual >= startColumn && visual < endColumn)
                    std::cout << (c == '\t' ? ' ' : c);
                ++visual;
            }
        };

        for (int i = 0; i < static_cast<int>(line.size());) {
            if (shay && line[i] == '!' && i + 1 < static_cast<int>(line.size()) && line[i + 1] == '#') {
                terminal_.magenta();
                emit(line[i++]);
                emit(line[i++]);
                terminal_.reset();
                continue;
            }
            if (line[i] == '"' || line[i] == '\'') {
                const char quote = line[i];
                terminal_.green();
                do {
                    const char c = line[i++];
                    emit(c);
                    if (c == quote) break;
                } while (i < static_cast<int>(line.size()));
                terminal_.reset();
                continue;
            }
            if (line[i] >= '0' && line[i] <= '9') {
                terminal_.yellow();
                while (i < static_cast<int>(line.size()) && line[i] >= '0' && line[i] <= '9')
                    emit(line[i++]);
                terminal_.reset();
                continue;
            }
            if (wordChar(line[i])) {
                int end = i;
                while (end < static_cast<int>(line.size()) && wordChar(line[end])) ++end;
                const std::string word = line.substr(i, end - i);
                const bool keyword = shay && (word == "ifit" || word == "elseit" || word == "exec" ||
                    word == "finish" || word == "forw" || word == "instxt" || word == "instxtexit" ||
                    word == "insval" || word == "$declr" || word == "$shaiiport");
                if (keyword) terminal_.cyan();
                while (i < end) emit(line[i++]);
                if (keyword) terminal_.reset();
                continue;
            }
            emit(line[i++]);
        }
        terminal_.reset();
    }
    void render() {
        auto [height, width] = terminal_.size();
        int bodyRows = std::max(1, height - 2);
        int gutter = std::max(4, static_cast<int>(std::to_string(rowOffset_ + height).size()) + 2);
        if (row_ < rowOffset_) rowOffset_ = row_;
        if (row_ >= rowOffset_ + bodyRows) rowOffset_ = row_ - bodyRows + 1;
        int textWidth = std::max(1, width - gutter);
        const int cursorVisualColumn = visualColumn(lines_[row_], col_);
        if (cursorVisualColumn < colOffset_) colOffset_ = cursorVisualColumn;
        if (cursorVisualColumn >= colOffset_ + textWidth)
            colOffset_ = cursorVisualColumn - textWidth + 1;

        terminal_.hideCursor();
        terminal_.move(1, 1); terminal_.reverse();
        std::string title = " YSEdit " + std::string(dirty_ ? "[+]" : "") + " " + filename_;
        if (static_cast<int>(title.size()) > width) title.resize(width);
        std::cout << title;
        for (int i = static_cast<int>(title.size()); i < width; ++i) std::cout << ' ';
        terminal_.reset();

        for (int screenRow = 0; screenRow < bodyRows; ++screenRow) {
            int fileRow = rowOffset_ + screenRow;
            terminal_.move(screenRow + 2, 1);
            if (fileRow < static_cast<int>(lines_.size())) {
                terminal_.dim();
                std::string number = std::to_string(fileRow + 1);
                std::cout << std::string(std::max(0, gutter - 2 - static_cast<int>(number.size())), ' ') << number << ' ';
                terminal_.reset();
                drawSyntax(lines_[fileRow], colOffset_, textWidth);
            } else { terminal_.dim(); std::cout << '~'; terminal_.reset(); }
            std::cout << "\x1b[K";
        }

        terminal_.move(height - 1, 1); terminal_.reverse();
        std::string status = " " + message_;
        if (static_cast<int>(status.size()) > width) status.resize(width);
        std::cout << status;
        for (int i = static_cast<int>(status.size()); i < width; ++i) std::cout << ' ';
        terminal_.reset();

        terminal_.move(height, 1);
        std::string position = " " + std::to_string(row_ + 1) + ":" + std::to_string(col_ + 1) + (dirty_ ? "  Modified" : "  Saved");
        if (static_cast<int>(position.size()) > width) position.resize(width);
        std::cout << position << "\x1b[K";
        terminal_.move(row_ - rowOffset_ + 2, gutter + cursorVisualColumn - colOffset_ + 1);
        terminal_.showCursor();
    }
};
} // namespace

int main(int argc, char** argv) {
    Editor editor(argc > 1 ? argv[1] : "");
    editor.run();
    return 0;
}
