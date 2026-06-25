#include "theme/colors.hpp"
#include <string>

namespace tui::extras {

    std::string get_color_sequence(const Color& color) {
        if (color.type == extras::Color::Type::ANSI) {
            return "\033[" + std::to_string(static_cast<int>(color.ansi_color)) + "m";
        }
        if (color.type == extras::Color::Type::RGB) {
            return "\033[38;2;" + std::to_string(color.r) + ";" + std::to_string(color.g) + ";" +
                std::to_string(color.b) + "m";
        }
        return "";
    }

} // namespace tui::extras
