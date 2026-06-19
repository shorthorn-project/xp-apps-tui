#include "core/input.hpp"

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif
#include <cstdio>
#include <unordered_map>

namespace tui {

    namespace {
        const std::unordered_map<int, Key> simple_map = {{'\n', Key::ENTER},  {'\r', Key::ENTER},
                                                         {' ', Key::SPACE},   {'\t', Key::TAB},
                                                         {8, Key::BACKSPACE}, {127, Key::BACKSPACE}};

        const std::unordered_map<int, Key> ansi_map = {
            {'A', Key::ARROW_UP},   {'B', Key::ARROW_DOWN}, {'C', Key::ARROW_RIGHT},
            {'D', Key::ARROW_LEFT}, {'H', Key::HOME},       {'F', Key::END},
            {'5', Key::PAGE_UP},    {'6', Key::PAGE_DOWN},  {'3', Key::KEY_DELETE}};

#ifdef _WIN32
        const std::unordered_map<int, Key> win_map = {
            {72, Key::ARROW_UP}, {80, Key::ARROW_DOWN}, {75, Key::ARROW_LEFT}, {77, Key::ARROW_RIGHT}, {71, Key::HOME},
            {79, Key::END},      {73, Key::PAGE_UP},    {81, Key::PAGE_DOWN},  {83, Key::KEY_DELETE}};
#endif

        std::pair<Key, char> read_ansi_sequence() {
            if (!Input::wait_for_input(10)) {
                return {Key::ESCAPE, 0};
            }

            const int ch1 = Input::get_key();
            if (ch1 == 27) { // ESCAPE
                return {Key::ESCAPE, 0};
            }

            if (ch1 == '[' || ch1 == 'O') {
                const int ch2 = Input::get_key();
                auto it = ansi_map.find(ch2);
                if (it != ansi_map.end()) {
                    if (ch2 == '5' || ch2 == '6' || ch2 == '3') {
                        Input::get_key();
                    }
                    return {it->second, 0};
                }
            }

            return {Key::UNKNOWN, 0};
        }
    } // namespace

    int Input::get_key() {
#ifdef _WIN32
        return _getch();
#else
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0) {
            return static_cast<unsigned char>(c);
        }
        return EOF;
#endif
    }

    bool Input::key_available() {
#ifdef _WIN32
        return _kbhit();
#else
        fd_set readfds;
        timeval timeout{};

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        const int result = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &timeout);
        return result > 0;
#endif
    }

    bool Input::wait_for_input(int timeout_ms) {
#ifdef _WIN32
        HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
        if (hInput == INVALID_HANDLE_VALUE) {
            return false;
        }
        DWORD result = WaitForSingleObject(hInput, static_cast<DWORD>(timeout_ms));
        return result == WAIT_OBJECT_0;
#else
        fd_set readfds;
        timeval timeout{};

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;

        const int result = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &timeout);
        return result > 0;
#endif
    }

    std::pair<Key, char> Input::get_input() {
        const int ch = get_key();

        if (ch == 27) {
            return read_ansi_sequence();
        }

        auto it = simple_map.find(ch);
        if (it != simple_map.end()) {
            return {it->second, 0};
        }

#ifdef _WIN32
        if (ch == 224) {
            const int next_ch = get_key();
            auto win_it = win_map.find(next_ch);
            if (win_it != win_map.end()) {
                return {win_it->second, 0};
            }
            return {Key::UNKNOWN, 0};
        }
#endif

        if (ch >= 32 && ch <= 126) {
            return {Key::NORMAL, static_cast<char>(ch)};
        }

        return {Key::UNKNOWN, 0};
    }

    Key Input::parse_escape_sequence() { return Key::UNKNOWN; }

} // namespace tui
