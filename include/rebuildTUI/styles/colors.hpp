#pragma once

#include <cstdint>
#include <tuple>
#include <vector>


namespace tui::extras {
    using color = std::tuple<uint8_t, uint8_t, uint8_t>;
    using v_styles = std::vector<color>;

    enum class BorderStyle { ROUNDED, DOUBLE, SHARP, ASCII };

    enum class AccentColor {
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
} // namespace tui::extras
