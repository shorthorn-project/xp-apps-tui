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

namespace tui {

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
        int ch = get_key();
        if (ch == 27) { // Escape
            if (wait_for_input(10)) {
                const int ch1 = get_key();
                if (ch1 == 27) {
                    return {Key::ESCAPE, 0};
                }

                if (ch1 == '[' || ch1 == 'O') {
                    switch (get_key()) {
                    case 'A':
                        return {Key::ARROW_UP, 0};
                    case 'B':
                        return {Key::ARROW_DOWN, 0};
                    case 'C':
                        return {Key::ARROW_RIGHT, 0};
                    case 'D':
                        return {Key::ARROW_LEFT, 0};
                    case 'H':
                        return {Key::HOME, 0};
                    case 'F':
                        return {Key::END, 0};
                    case '5':
                        get_key();
                        return {Key::PAGE_UP, 0};
                    case '6':
                        get_key();
                        return {Key::PAGE_DOWN, 0};
                    case '3':
                        get_key();
                        return {Key::KEY_DELETE, 0};
                    default:
                        return {Key::UNKNOWN, 0};
                    }
                }
            }
            return {Key::ESCAPE, 0};
        }

        switch (ch) {
        case '\n':
        case '\r':
            return {Key::ENTER, 0};
        case ' ':
            return {Key::SPACE, 0};
        case '\t':
            return {Key::TAB, 0};
        case 8:
        case 127:
            return {Key::BACKSPACE, 0};
#ifdef _WIN32
        case 224:
            ch = get_key();
            switch (ch) {
            case 72:
                return {Key::ARROW_UP, 0};
            case 80:
                return {Key::ARROW_DOWN, 0};
            case 75:
                return {Key::ARROW_LEFT, 0};
            case 77:
                return {Key::ARROW_RIGHT, 0};
            case 71:
                return {Key::HOME, 0};
            case 79:
                return {Key::END, 0};
            case 73:
                return {Key::PAGE_UP, 0};
            case 81:
                return {Key::PAGE_DOWN, 0};
            case 83:
                return {Key::KEY_DELETE, 0};
            default:
                return {Key::UNKNOWN, 0};
            }
#endif
        default:
            return {(ch >= 32 && ch <= 126) ? Key::NORMAL : Key::UNKNOWN,
                    (ch >= 32 && ch <= 126) ? static_cast<char>(ch) : 0};
        }
    }

    Key Input::parse_escape_sequence() {
        return Key::UNKNOWN;
    }

} // namespace tui
