#pragma once

#include <utility>

namespace tui {

    /**
     * @brief Special key codes for navigation
     */
    enum class Key {
        UNKNOWN = 0,
        ARROW_UP = 1,
        ARROW_DOWN = 2,
        ARROW_LEFT = 3,
        ARROW_RIGHT = 4,
        ENTER = 5,
        SPACE = 6,
        ESCAPE = 7,
        BACKSPACE = 8,
        TAB = 9,
        HOME = 10,
        END = 11,
        PAGE_UP = 12,
        PAGE_DOWN = 13,
        KEY_DELETE = 14,
        KEY_J = 15,
        KEY_K = 16,
        KEY_H = 17,
        KEY_L = 18,
        NORMAL = 19,
        F1 = 20,
        F2 = 21,
        F3 = 22,
        F4 = 23,
        F5 = 24,
        F6 = 25,
        F7 = 26,
        F8 = 27,
        F9 = 28,
        F10 = 29,
        F11 = 30,
        F12 = 31
    };

    /**
     * @brief Key event structure
     */
    struct KeyEvent {
        Key key;
        char character;

        explicit KeyEvent(const Key k = Key::UNKNOWN, const char c = '\0') : key(k), character(c) {}
    };

    class Input {
    public:
        static int get_key();
        static bool key_available();
        static std::pair<Key, char> get_input();
        static bool wait_for_input(int timeout_ms);

    private:
        static Key parse_escape_sequence();
    };

} // namespace tui
