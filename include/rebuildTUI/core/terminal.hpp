#pragma once

#include <optional>
#include <string>
#include "core/input.hpp"
#include "theme/colors.hpp"
#include "theme/gradient.hpp"
// #include <iostream>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#endif

namespace tui {

    /**
     * @brief Terminal control utilities for cross-platform TUI applications
     */
    class TerminalUtils {
    public:
        // Key enum moved to tui::Key in core/input.hpp

        /**
         * @brief Color codes for terminal output
         */
        enum class Color {
            RESET = 0,
            BLACK = 30,
            RED = 31,
            GREEN = 32,
            YELLOW = 33,
            BLUE = 34,
            MAGENTA = 35,
            CYAN = 36,
            WHITE = 37,
            BRIGHT_BLACK = 90,
            BRIGHT_RED = 91,
            BRIGHT_GREEN = 92,
            BRIGHT_YELLOW = 93,
            BRIGHT_BLUE = 94,
            BRIGHT_MAGENTA = 95,
            BRIGHT_CYAN = 96,
            BRIGHT_WHITE = 97
        };

        /**
         * @brief Text style codes
         */
        enum class Style {
            RESET = 0,
            BOLD = 1,
            DIM = 2,
            ITALIC = 3,
            UNDERLINE = 4,
            BLINK = 5,
            REVERSE = 7,
            STRIKETHROUGH = 9
        };

        static void init_terminal();
        static void restore_terminal();
        static void clear_screen();
        static void move_cursor(int row, int col);
        static void hide_cursor();
        static void show_cursor();
        static std::pair<int, int> get_terminal_size();
        static void set_color(Color color);
        static void set_color(extras::AccentColor color);
        static size_t get_visible_string_length(const std::string& string);

        // Gradient
        static void set_color_rgb(uint8_t r, uint8_t g, uint8_t b);
        static void set_color_rgb(extras::GradientColor color);

        static void set_style(Style style);
        static void reset_formatting();

        static void draw_horizontal_line(int row, int start_col, int length, char ch = '-');
        static void draw_vertical_line(int start_row, int col, int length, char ch = '|');
        static void draw_box(int top_row, int left_col, int width, int height);
        static void print_centered(const std::string& text, int width, int row = -1);
        static void print_at(int row, int col, const std::string& text);
        static int get_centered_col(int content_width);
        static int get_centered_row(int content_height);
        static std::pair<int, int> get_centered_position(int content_width, int content_height);
        static void print_centered_at_row(int row, const std::string& text);
        static void print_centered_screen(const std::string& text);
        static void draw_centered_box(int width, int height);
        static std::pair<int, int> get_centering_margins(int content_width, int content_height);
        static void save_cursor_position();
        static void restore_cursor_position();
        static void set_echo(bool enable);
        static void set_canonical_mode(bool enable);
        static void flush();

    private:
#ifdef _WIN32
        static HANDLE hConsole;
        static CONSOLE_SCREEN_BUFFER_INFO csbi;
        static DWORD originalConsoleMode;
        static bool is_wt;
#else
        static struct termios original_termios;
        static bool termios_saved;
#endif
        static void init_platform_terminal();
        static void restore_platform_terminal();
    };

    /**
     * @brief Terminal manager class for instance-based terminal control
     */
    class TerminalManager {
    private:
        bool terminal_initialized_;

    public:
        TerminalManager() : terminal_initialized_(false) {}
        ~TerminalManager() {
            if (terminal_initialized_) {
                restore_terminal();
            }
        }

        void setup_terminal() {
            if (!terminal_initialized_) {
                TerminalUtils::init_terminal();
                terminal_initialized_ = true;
            }
        }

        void restore_terminal() {
            if (terminal_initialized_) {
                TerminalUtils::restore_terminal();
                terminal_initialized_ = false;
            }
        }

        static void clear_screen() { TerminalUtils::clear_screen(); }
        static void flush_output() { TerminalUtils::flush(); }

        static std::optional<tui::KeyEvent> get_key_input();

        static bool wait_for_input(int timeout_ms);
        static bool key_available();
        static std::pair<int, int> get_terminal_size() { return TerminalUtils::get_terminal_size(); }
    };

} // namespace tui
