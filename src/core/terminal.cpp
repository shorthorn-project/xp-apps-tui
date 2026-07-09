#include "core/terminal.hpp"
#include <fmt/base.h>
#include "core/input.hpp"

#include <sstream> // IWYU pragma: keep
#include <vector>

#ifndef _WIN32
#include <sys/ioctl.h>
#endif

#include <unistd.h>

namespace tui {

#ifdef _WIN32
    HANDLE TerminalUtils::hConsole = INVALID_HANDLE_VALUE;
    CONSOLE_SCREEN_BUFFER_INFO TerminalUtils::csbi = {};
    DWORD TerminalUtils::originalConsoleMode = 0;
    bool TerminalUtils::is_wt = false;
    ConsoleType TerminalUtils::s_console_type = ConsoleType::LegacyRaster;

    namespace {
        WORD convert_ansi_attrs(const std::vector<int>& codes, WORD current_attrs) {
            WORD bg_mask = current_attrs & (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY);
            WORD fg_mask = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
            bool bold = false;
            bool reverse = false;

            for (int code : codes) {
                if (code == 0) {
                    fg_mask = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
                    bold = false;
                    reverse = false;
                } else if (code == 1) {
                    bold = true;
                } else if (code == 7) {
                    reverse = true;
                } else if (code >= 30 && code <= 37) {
                    fg_mask = 0;
                    int c = code - 30;
                    if (c & 1) {
                        fg_mask |= FOREGROUND_RED;
                    }
                    if (c & 2) {
                        fg_mask |= FOREGROUND_GREEN;
                    }
                    if (c & 4) {
                        fg_mask |= FOREGROUND_BLUE;
                    }
                } else if (code >= 90 && code <= 97) {
                    fg_mask = FOREGROUND_INTENSITY;
                    int c = code - 90;
                    if (c & 1) {
                        fg_mask |= FOREGROUND_RED;
                    }
                    if (c & 2) {
                        fg_mask |= FOREGROUND_GREEN;
                    }
                    if (c & 4) {
                        fg_mask |= FOREGROUND_BLUE;
                    }
                }
            }

            if (bold && !reverse) {
                fg_mask |= FOREGROUND_INTENSITY;
            }

            if (reverse) {
                WORD new_fg = (bg_mask >> 4) & 0x0F;
                WORD new_bg = (fg_mask << 4) & 0xF0;
                return new_fg | new_bg;
            }

            return fg_mask | bg_mask;
        }
    } // namespace
#else
    termios TerminalUtils::original_termios = {};
    bool TerminalUtils::termios_saved = false;
#endif

    void TerminalUtils::init_terminal() {
        init_platform_terminal();
        clear_screen();
        hide_cursor();
    }

    void TerminalUtils::restore_terminal() {
        show_cursor();
        reset_formatting();
        restore_platform_terminal();
    }

    void TerminalUtils::clear_screen() {
#ifdef _WIN32
        if (s_console_type != ConsoleType::VT) {
            if (hConsole != INVALID_HANDLE_VALUE) {
                COORD coord = {0, 0};
                DWORD written;
                GetConsoleScreenBufferInfo(hConsole, &csbi);
                FillConsoleOutputCharacterA(hConsole, ' ', csbi.dwSize.X * csbi.dwSize.Y, coord, &written);
                FillConsoleOutputAttribute(hConsole, csbi.wAttributes, csbi.dwSize.X * csbi.dwSize.Y, coord, &written);
                SetConsoleCursorPosition(hConsole, coord);
            }
            return;
        }
#endif
        fmt::print("\033[2J\033[H");
        flush();
    }

    void TerminalUtils::move_cursor(int row, int col) {
#ifdef _WIN32
        if (s_console_type != ConsoleType::VT) {
            if (hConsole != INVALID_HANDLE_VALUE) {
                COORD coord = {static_cast<SHORT>(col - 1), static_cast<SHORT>(row - 1)};
                SetConsoleCursorPosition(hConsole, coord);
            }
            return;
        }
#endif
        fmt::print("\033[{};{}H", row, col);
        flush();
    }

    void TerminalUtils::hide_cursor() {
#ifdef _WIN32
        if (s_console_type != ConsoleType::VT) {
            if (hConsole != INVALID_HANDLE_VALUE) {
                CONSOLE_CURSOR_INFO cursorInfo;
                GetConsoleCursorInfo(hConsole, &cursorInfo);
                cursorInfo.bVisible = FALSE;
                SetConsoleCursorInfo(hConsole, &cursorInfo);
            }
            return;
        }
#endif
        fmt::print("\033[?25l");
        flush();
    }

    void TerminalUtils::show_cursor() {
#ifdef _WIN32
        if (s_console_type != ConsoleType::VT) {
            if (hConsole != INVALID_HANDLE_VALUE) {
                CONSOLE_CURSOR_INFO cursorInfo;
                GetConsoleCursorInfo(hConsole, &cursorInfo);
                cursorInfo.bVisible = TRUE;
                SetConsoleCursorInfo(hConsole, &cursorInfo);
            }
            return;
        }
#endif
        fmt::print("\033[?25h");
        flush();
    }

    std::pair<int, int> TerminalUtils::get_terminal_size() {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            GetConsoleScreenBufferInfo(hConsole, &csbi);
            int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            int height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
            return {height, width};
        }
        return {25, 80}; // Default fallback
#else
        winsize w{};
        // if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        //   return {w.ws_row, w.ws_col};
        // }
        // return {25, 80}; // Default fallback

        return ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0
            ? std::make_pair(static_cast<int>(w.ws_row), static_cast<int>(w.ws_col))
            : std::make_pair(25, 80);
#endif
    }

    void TerminalUtils::set_color(Color color) {
#ifdef _WIN32
        if (s_console_type != ConsoleType::VT) {
            if (hConsole != INVALID_HANDLE_VALUE) {
                WORD attributes = 0;
                switch (color) {
                case Color::BLACK:
                    attributes = 0;
                    break;
                case Color::RED:
                    attributes = FOREGROUND_RED;
                    break;
                case Color::GREEN:
                    attributes = FOREGROUND_GREEN;
                    break;
                case Color::YELLOW:
                    attributes = FOREGROUND_RED | FOREGROUND_GREEN;
                    break;
                case Color::BLUE:
                    attributes = FOREGROUND_BLUE;
                    break;
                case Color::MAGENTA:
                    attributes = FOREGROUND_RED | FOREGROUND_BLUE;
                    break;
                case Color::CYAN:
                    attributes = FOREGROUND_GREEN | FOREGROUND_BLUE;
                    break;
                case Color::WHITE:
                    attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                    break;
                case Color::BRIGHT_BLACK:
                    attributes = FOREGROUND_INTENSITY;
                    break;
                case Color::BRIGHT_RED:
                    attributes = FOREGROUND_RED | FOREGROUND_INTENSITY;
                    break;
                case Color::BRIGHT_GREEN:
                    attributes = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                    break;
                case Color::BRIGHT_YELLOW:
                    attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                    break;
                case Color::BRIGHT_BLUE:
                    attributes = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                    break;
                case Color::BRIGHT_MAGENTA:
                    attributes = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                    break;
                case Color::BRIGHT_CYAN:
                    attributes = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                    break;
                case Color::BRIGHT_WHITE:
                    attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                    break;
                default:
                    attributes = csbi.wAttributes;
                    break;
                }
                SetConsoleTextAttribute(hConsole, attributes);
            }
            return;
        }
#endif
        fmt::print("\033[{}m", (color == Color::RESET) ? 0 : static_cast<int>(color));
        flush();
    }

    void TerminalUtils::set_color(extras::AccentColor color) {
#ifdef _WIN32
        if (s_console_type != ConsoleType::VT) {
            if (hConsole != INVALID_HANDLE_VALUE) {
                WORD attributes = 0;
                switch (color) {
                case extras::AccentColor::BLACK:
                    attributes = 0;
                    break;
                case extras::AccentColor::RED:
                    attributes = FOREGROUND_RED;
                    break;
                case extras::AccentColor::GREEN:
                    attributes = FOREGROUND_GREEN;
                    break;
                case extras::AccentColor::YELLOW:
                    attributes = FOREGROUND_RED | FOREGROUND_GREEN;
                    break;
                case extras::AccentColor::BLUE:
                    attributes = FOREGROUND_BLUE;
                    break;
                case extras::AccentColor::MAGENTA:
                    attributes = FOREGROUND_RED | FOREGROUND_BLUE;
                    break;
                case extras::AccentColor::CYAN:
                    attributes = FOREGROUND_GREEN | FOREGROUND_BLUE;
                    break;
                case extras::AccentColor::WHITE:
                    attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                    break;
                case extras::AccentColor::BRIGHT_BLACK:
                    attributes = FOREGROUND_INTENSITY;
                    break;
                case extras::AccentColor::BRIGHT_RED:
                    attributes = FOREGROUND_RED | FOREGROUND_INTENSITY;
                    break;
                case extras::AccentColor::BRIGHT_GREEN:
                    attributes = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                    break;
                case extras::AccentColor::BRIGHT_YELLOW:
                    attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                    break;
                case extras::AccentColor::BRIGHT_BLUE:
                    attributes = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                    break;
                case extras::AccentColor::BRIGHT_MAGENTA:
                    attributes = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                    break;
                case extras::AccentColor::BRIGHT_CYAN:
                    attributes = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                    break;
                case extras::AccentColor::BRIGHT_WHITE:
                    attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                    break;
                default:
                    attributes = csbi.wAttributes;
                    break;
                }
                SetConsoleTextAttribute(hConsole, attributes);
            }
            return;
        }
#endif
        fmt::print("\033[{}m", (color == extras::AccentColor::RESET) ? 0 : static_cast<int>(color));
        flush();
    }

    size_t TerminalUtils::get_visible_string_length(const std::string& string) {
        size_t length = 0;
        auto in_escape = false;

        for (size_t i = 0; i < string.length(); ++i) {
            const unsigned char c = static_cast<unsigned char>(string[i]);

            if (c == '\033') {
                in_escape = true;
            }

            if (!in_escape) {
                // Simple UTF-8 check: only increment for the first byte of a sequence
                // (bits 10xxxxxx are continuation bytes)
                if ((c & 0xC0) != 0x80) {
                    length++;
                }
            }

            if (in_escape && c == 'm') {
                in_escape = false;
            }
        }

        return length;
    }

    bool TerminalUtils::is_vt_supported() {
#ifdef _WIN32
        return s_console_type == ConsoleType::VT;
#else // *NIX
        return true;
#endif
    }

    void TerminalUtils::set_color_rgb(uint8_t r, uint8_t g, uint8_t b) {
        fmt::print("\033[38;2;{};{};{}m", static_cast<int>(r), static_cast<int>(g), static_cast<int>(b));
        flush();
    }

    void TerminalUtils::set_color_rgb(const extras::GradientColor color) {
#ifdef _WIN32
        if (s_console_type != ConsoleType::VT) {
            return;
        }
#endif
        auto [r, g, b] = color.get_color();
        set_color_rgb(r, g, b);
    }

    void TerminalUtils::set_style(Style style) {
#ifdef _WIN32
        if (s_console_type != ConsoleType::VT) {
            if (hConsole != INVALID_HANDLE_VALUE) {
                switch (style) {
                case Style::BOLD:
                    SetConsoleTextAttribute(hConsole, csbi.wAttributes | FOREGROUND_INTENSITY);
                    break;
                case Style::REVERSE:
                    SetConsoleTextAttribute(hConsole,
                                            ((csbi.wAttributes & 0xF0) >> 4) | ((csbi.wAttributes & 0x0F) << 4));
                    break;
                case Style::RESET:
                    SetConsoleTextAttribute(hConsole, csbi.wAttributes);
                    break;
                default:
                    break; // Other styles not supported on Windows
                }
            }
            return;
        }
#endif
        fmt::print("\033[{}m", static_cast<int>(style));
        flush();
    }

    void TerminalUtils::reset_formatting() {
#ifdef _WIN32
        if (s_console_type != ConsoleType::VT) {
            if (hConsole != INVALID_HANDLE_VALUE) {
                SetConsoleTextAttribute(hConsole, csbi.wAttributes);
            }
            return;
        }
#endif
        fmt::print("\033[0m");
        flush();
    }

    void TerminalUtils::draw_horizontal_line(const int row, const int start_col, const int length, const char ch) {
        move_cursor(row, start_col);
        // for (auto i = 0; i < length; ++i) {
        //     fmt::print("{}", ch);
        // }
        std::string line(length, ch);
        print_safe(line);
    }

    void TerminalUtils::draw_vertical_line(int start_row, int col, int length, char ch) {
        std::string str(1, ch);
        for (int i = 0; i < length; ++i) {
            move_cursor(start_row + i, col);
            // fmt::print("{}", ch);
            print_safe(str);
        }

        flush();
    }

    void TerminalUtils::draw_box(int top_row, int left_col, int width, int height) {
        // Top border
        move_cursor(top_row, left_col);
        print_safe("+");
        std::string top_line(width - 2, '-');
        print_safe(top_line);
        print_safe("+");

        // Side borders
        for (int i = 1; i < height - 1; ++i) {
            move_cursor(top_row + i, left_col);
            print_safe("|");
            move_cursor(top_row + i, left_col + width - 1);
            print_safe("|");
        }

        // Bottom border
        move_cursor(top_row + height - 1, left_col);
        print_safe("+");
        std::string bottom_line(width - 2, '-');
        print_safe(bottom_line);
        print_safe("+");
    }

    void TerminalUtils::print_centered(const std::string& text, int width, int row) {
        int padding = (width - static_cast<int>(text.length())) / 2;
        std::string padded_text = std::string(std::max(0, padding), ' ') + text;

        if (row >= 0) {
            move_cursor(row, 1);
        }

        print_safe(padded_text);
    }

    void TerminalUtils::print_at(int row, int col, const std::string& text) {
        move_cursor(row, col);
        print_safe(text);
    }

    void TerminalUtils::save_cursor_position() {
#ifdef _WIN32
        if (s_console_type != ConsoleType::VT) {
            if (hConsole != INVALID_HANDLE_VALUE) {
                GetConsoleScreenBufferInfo(hConsole, &csbi);
            }
            return;
        }
#endif
        fmt::print("\033[s");
        flush();
    }

    void TerminalUtils::restore_cursor_position() {
#ifdef _WIN32
        if (s_console_type != ConsoleType::VT) {
            if (hConsole != INVALID_HANDLE_VALUE) {
                SetConsoleCursorPosition(hConsole, csbi.dwCursorPosition);
            }
            return;
        }
#endif
        fmt::print("\033[u");
        flush();
    }

    void TerminalUtils::set_echo(bool enable) {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            DWORD mode;
            GetConsoleMode(hConsole, &mode);
            mode = enable ? mode | ENABLE_ECHO_INPUT : mode & ~ENABLE_ECHO_INPUT;
            SetConsoleMode(hConsole, mode);
        }
#else
        if (termios_saved) {
            struct termios new_termios = original_termios;
            new_termios.c_lflag = enable ? new_termios.c_lflag | ECHO : new_termios.c_lflag & ~ECHO;
            tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
        }
#endif
    }

    void TerminalUtils::set_canonical_mode(bool enable) {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            DWORD mode;
            GetConsoleMode(hConsole, &mode);
            mode = enable ? mode | ENABLE_LINE_INPUT : mode & ~ENABLE_LINE_INPUT;
            SetConsoleMode(hConsole, mode);
        }
#else
        if (termios_saved) {
            struct termios new_termios = original_termios;
            new_termios.c_lflag = enable ? new_termios.c_lflag | ICANON : new_termios.c_lflag & ~ICANON;
            tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
        }
#endif
    }

    void TerminalUtils::flush() { std::fflush(stdout); }

    int TerminalUtils::get_centered_col(int content_width) {
        auto [height, width] = get_terminal_size();
        return std::max(1, (width - content_width) / 2 + 1);
    }

    int TerminalUtils::get_centered_row(int content_height) {
        auto [height, width] = get_terminal_size();
        return std::max(1, (height - content_height) / 2 + 1);
    }

    std::pair<int, int> TerminalUtils::get_centered_position(int content_width, int content_height) {
        return {get_centered_row(content_height), get_centered_col(content_width)};
    }

    void TerminalUtils::print_centered_at_row(int row, const std::string& text) {
        int col = get_centered_col(static_cast<int>(text.length()));
        print_at(row, col, text);
    }

    void TerminalUtils::print_centered_screen(const std::string& text) {
        int row = get_centered_row(1);
        int col = get_centered_col(static_cast<int>(text.length()));
        print_at(row, col, text);
    }

    void TerminalUtils::draw_centered_box(int box_width, int box_height) {
        auto [row, col] = get_centered_position(box_width, box_height);
        draw_box(row, col, box_width, box_height);
    }

    std::pair<int, int> TerminalUtils::get_centering_margins(int content_width, int content_height) {
        auto [height, width] = get_terminal_size();
        int horizontal_margin = std::max(0, (width - content_width) / 2);
        int vertical_margin = std::max(0, (height - content_height) / 2);
        return {horizontal_margin, vertical_margin};
    }

    void TerminalUtils::init_platform_terminal() {
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole == INVALID_HANDLE_VALUE) {
            s_console_type = ConsoleType::LegacyRaster;
            return;
        }

        // win 4.x check
        DWORD dwVersion = GetVersion();
        bool is_win9x = (dwVersion & 0x80000000) != 0;

        if (is_win9x) {
            s_console_type = ConsoleType::LegacyRaster;
        } else {
            DWORD originalOutMode = 0;
            if (!GetConsoleMode(hConsole, &originalOutMode)) {
                s_console_type = ConsoleType::LegacyRaster;
                return;
            }

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
            DWORD newOutMode = originalOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (SetConsoleMode(hConsole, newOutMode)) {
                s_console_type = ConsoleType::VT;
            } else {
                s_console_type = ConsoleType::LegacyRaster;

                HMODULE hKernel = GetModuleHandleA("kernel32.dll");
                if (hKernel) {
                    typedef BOOL(WINAPI * GetCurrentConsoleFontEx_t)(HANDLE, BOOL, PVOID);
                    // FIXME: Cast through void* to silence `-Wcast-function-type warning`
                    auto pGetFontEx =
                        (GetCurrentConsoleFontEx_t)(void*)GetProcAddress(hKernel, "GetCurrentConsoleFontEx");
                    if (pGetFontEx) {
                        struct MY_CONSOLE_FONT_INFOEX {
                            ULONG cbSize;
                            DWORD nFont;
                            COORD dwFontSize;
                            UINT FontFamily;
                            UINT FontWeight;
                            WCHAR FaceName[32];
                        };
                        // FIXME: prevents `-Wmissing-field-initializers` warning
                        MY_CONSOLE_FONT_INFOEX fontInfo = {};
                        fontInfo.cbSize = sizeof(fontInfo);

                        if (pGetFontEx(hConsole, FALSE, &fontInfo)) {
                            if (fontInfo.FontFamily & 0x04) { // TMPF_TRUETYPE
                                s_console_type = ConsoleType::LegacyUnicode;
                            }
                        }
                    }
                }
            }
        }

        HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
        if (hInput != INVALID_HANDLE_VALUE) {
            GetConsoleMode(hInput, &originalConsoleMode);
            SetConsoleMode(hInput, ENABLE_PROCESSED_INPUT | ENABLE_WINDOW_INPUT);
        }

        GetConsoleScreenBufferInfo(hConsole, &csbi);
        is_wt = std::getenv("WT_SESSION") ? true : false;
#else
        if (!termios_saved) {
            if (tcgetattr(STDIN_FILENO, &original_termios) == 0) {
                termios_saved = true;
            }
        }

        if (termios_saved) {
            struct termios new_termios = original_termios;
            new_termios.c_lflag &= ~(ICANON | ECHO);
            new_termios.c_iflag &= ~ICRNL;
            new_termios.c_cc[VMIN] = 1;
            new_termios.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
            setvbuf(stdout, nullptr, _IONBF, 0);
        }
#endif
    }

    void TerminalUtils::restore_platform_terminal() {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
            if (hInput != INVALID_HANDLE_VALUE) {
                SetConsoleMode(hInput, originalConsoleMode);
            }
        }
#else
        if (termios_saved) {
            tcflush(STDIN_FILENO, TCIFLUSH);
            tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
            setvbuf(stdout, nullptr, _IOLBF, BUFSIZ); // Restore line buffering
            termios_saved = false;
        }
#endif
    }

    bool TerminalUtils::is_unsupported_emoji(unsigned int codepoint) {
        // Above BMP (https://www.unicode.org/glossary/#basic_multilingual_plane)
        if (codepoint > 0xFFFF) {
            return true;
        }

        // Dingbats block (https://www.unicode.org/charts/PDF/U2700.pdf)
        if (codepoint >= 0x2700 && codepoint <= 0x27BF) {
            return true;
        }

        // https://www.unicode.org/charts/PDF/U2600.pdf
        if (codepoint >= 0x2600 && codepoint <= 0x26FF) {
            // IBM CP437 Smileys (https://en.wikipedia.org/wiki/Code_page_437)
            if (codepoint == 0x263A || codepoint == 0x263B) {
                return false;
            }
            return true;
        }

        return false;
    }

    std::wstring TerminalUtils::utf8_to_safe_wstring(const std::string& utf8_str, ConsoleType type) {
        std::vector<unsigned int> codepoints;
        codepoints.reserve(utf8_str.size());

        // https://tools.ietf.org/html/rfc3629#section-3
        for (size_t i = 0; i < utf8_str.size();) {
            unsigned char c = utf8_str[i];
            unsigned int cp = 0;
            size_t len = 0;

            if (c <= 0x7F) {
                cp = c;
                len = 1;
            } else if ((c & 0xE0) == 0xC0) {
                cp = c & 0x1F;
                len = 2;
            } else if ((c & 0xF0) == 0xE0) {
                cp = c & 0x0F;
                len = 3;
            } else if ((c & 0xF8) == 0xF0) {
                cp = c & 0x07;
                len = 4;
            } else {
                i++;
                continue;
            }

            if (i + len > utf8_str.size()) {
                break;
            }

            for (size_t j = 1; j < len; ++j) {
                cp = (cp << 6) | (utf8_str[i + j] & 0x3F);
            }
            codepoints.push_back(cp);
            i += len;
        }

        std::wstring result;
        result.reserve(codepoints.size());

        for (unsigned int cp : codepoints) {
            if (type == ConsoleType::VT) {
                if (cp <= 0xFFFF) {
                    result.push_back(static_cast<wchar_t>(cp));
                } else {
                    // UTF-16 (https://tools.ietf.org/html/rfc2781#section-2.1)
                    cp -= 0x10000;
                    result.push_back(static_cast<wchar_t>((cp >> 10) + 0xD800));
                    result.push_back(static_cast<wchar_t>((cp & 0x3FF) + 0xDC00));
                }
                continue;
            }

            // skip unicode variation selectors (U+FE00 - U+FE0F)
            if (cp >= 0xFE00 && cp <= 0xFE0F) {
                continue;
            }

            // rounded borders not supported in CP437 and CP866, so here is dirty-hack to replace to sharp borders
            if (cp == 0x256D) {
                cp = 0x250C; // ╭ -> ┌
            } else if (cp == 0x256E) {
                cp = 0x2510; // ╮ -> ┐
            } else if (cp == 0x256F) {
                cp = 0x2518; // ╯ -> ┘
            } else if (cp == 0x2570) {
                cp = 0x2514; // ╰ -> └
            }

            if (is_unsupported_emoji(cp)) {
                if (cp == 0x2705 || cp == 0x2713) { // check mark
                    result.push_back(type == ConsoleType::LegacyUnicode ? L'\x221A' : L'v');
                } else if (cp == 0x274C || cp == 0x2717) { // cross mark
                    result.push_back(L'X');
                } else {
                    if (type == ConsoleType::LegacyUnicode) {
                        result.push_back(L'\x25A0'); // black square
                    } else {
                        result.push_back(L'*');
                    }
                }
            } else {
                if (cp <= 0xFFFF) {
                    result.push_back(static_cast<wchar_t>(cp));
                }
            }
        }
        return result;
    }

    std::string TerminalUtils::utf16_to_oem(const std::wstring& wstr) {
        if (wstr.empty()) {
            return "";
        }

        int size_r = WideCharToMultiByte(CP_OEMCP, 0, wstr.c_str(), static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
        if (size_r <= 0) {
            return "";
        }

        std::vector<char> buffer(size_r);
        WideCharToMultiByte(CP_OEMCP, 0, wstr.c_str(), static_cast<int>(wstr.size()), buffer.data(), size_r, NULL,
                            NULL);
        return std::string(buffer.begin(), buffer.end());
    }

    std::string TerminalUtils::strip_ansi(const std::string& str) {
        std::string result;
        result.reserve(str.size());
        bool in_escape = false;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '\033') {
                in_escape = true;
                continue;
            }
            if (in_escape) {
                if (str[i] == 'm') {
                    in_escape = false;
                }
                continue;
            }
            result.push_back(str[i]);
        }
        return result;
    }

    void TerminalUtils::print_safe(const std::string& utf8_str) {
#ifdef _WIN32
        if (s_console_type == ConsoleType::VT) {
            fmt::print("{}", utf8_str);
            flush();
            return;
        }

        if (utf8_str.find('\033') == std::string::npos) {
            std::wstring wstr = utf8_to_safe_wstring(utf8_str, s_console_type);
            if (s_console_type == ConsoleType::LegacyUnicode) {
                DWORD written;
                WriteConsoleW(hConsole, wstr.c_str(), static_cast<DWORD>(wstr.size()), &written, NULL);
            } else {
                std::string oem_str = utf16_to_oem(wstr);
                std::fwrite(oem_str.data(), 1, oem_str.size(), stdout);
            }
            flush();
            return;
        }

        // ansi -> windows console api
        size_t i = 0;
        while (i < utf8_str.size()) {
            if (utf8_str[i] == '\033') {
                if (i + 1 < utf8_str.size() && utf8_str[i + 1] == '[') {
                    size_t j = i + 2;

                    // 0x40 - 0x7E => CSI command
                    while (j < utf8_str.size() && (utf8_str[j] < 0x40 || utf8_str[j] > 0x7E)) {
                        j++;
                    }

                    if (j < utf8_str.size()) {
                        char cmd_char = utf8_str[j];
                        std::string seq = utf8_str.substr(i + 2, j - (i + 2));

                        if (cmd_char == 'm') {
                            std::vector<int> codes;
                            std::stringstream ss(seq);
                            std::string token;
                            while (std::getline(ss, token, ';')) {
                                if (!token.empty()) {
                                    try {
                                        codes.push_back(std::stoi(token));
                                    } catch (...) {
                                    }
                                }
                            }
                            if (codes.empty()) {
                                codes.push_back(0);
                            }

                            if (hConsole != INVALID_HANDLE_VALUE) {
                                CONSOLE_SCREEN_BUFFER_INFO temp_csbi;
                                if (GetConsoleScreenBufferInfo(hConsole, &temp_csbi)) {
                                    WORD new_attrs = convert_ansi_attrs(codes, temp_csbi.wAttributes);
                                    SetConsoleTextAttribute(hConsole, new_attrs);
                                }
                            }
                        }

                        i = j + 1;
                        continue;
                    }
                }
            }

            size_t next_esc = utf8_str.find('\033', i);
            if (next_esc == std::string::npos) {
                next_esc = utf8_str.size();
            }

            std::string chunk = utf8_str.substr(i, next_esc - i);
            if (!chunk.empty()) {
                std::wstring wstr = utf8_to_safe_wstring(chunk, s_console_type);
                if (s_console_type == ConsoleType::LegacyUnicode) {
                    DWORD written;
                    WriteConsoleW(hConsole, wstr.c_str(), static_cast<DWORD>(wstr.size()), &written, NULL);
                } else {
                    std::string oem_str = utf16_to_oem(wstr);
                    std::fwrite(oem_str.data(), 1, oem_str.size(), stdout);
                }
            }
            i = next_esc;
        }
#else
        fmt::print("{}", utf8_str);
#endif
        flush();
    }

    std::optional<tui::KeyEvent> TerminalManager::get_key_input() {
        if (!Input::key_available()) {
            return std::nullopt;
        }

        auto [key, character] = Input::get_input();
        return tui::KeyEvent(key, character);
    }

    bool TerminalManager::wait_for_input(int timeout_ms) { return Input::wait_for_input(timeout_ms); }

    bool TerminalManager::key_available() { return Input::key_available(); }

} // namespace tui
