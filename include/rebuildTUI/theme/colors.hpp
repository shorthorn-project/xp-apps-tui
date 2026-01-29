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

    struct Color {
        enum class Type { ANSI, RGB };
        Type type = Type::ANSI;
        AccentColor ansi_color = AccentColor::RESET;
        uint8_t r = 0, g = 0, b = 0;

        Color() = default;
        Color(AccentColor c) : type(Type::ANSI), ansi_color(c) {} // Implicit conversion
        Color(uint8_t red, uint8_t green, uint8_t blue) : type(Type::RGB), r(red), g(green), b(blue) {}

        static Color FromRGB(uint8_t r, uint8_t g, uint8_t b) { return Color(r, g, b); }
        static Color Ansi(AccentColor c) { return Color(c); }
    };

    struct ColorPalette {
        Color border = AccentColor::WHITE;
        Color header_text = AccentColor::CYAN;
        Color header_border = AccentColor::WHITE;
        Color section_name = AccentColor::WHITE;
        Color item_name = AccentColor::WHITE;
        Color selected_item = AccentColor::CYAN;
        Color unselected_item = AccentColor::WHITE;
        Color counter = AccentColor::BRIGHT_BLACK;
        Color footer = AccentColor::BRIGHT_BLACK;
    };
} // namespace tui::extras
