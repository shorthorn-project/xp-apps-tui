#pragma once

#include <format>
#include <sstream>
#include <string>

namespace tui::extras {

    template <typename... Args>
    auto set_style(const std::string& text, Args... styles) {
        std::stringstream st_s;
        st_s << "\033[";
        ((st_s << static_cast<int>(styles) << ";"), ...);
        std::string style_codes = st_s.str();
        style_codes.pop_back();

        return style_codes + "m" + text + "\033[0m";
    }

    enum class TextMode {
        NORMAL = 0,
        BOLD = 1,
        FAINT = 2,
        ITALIC = 3,
        UNDERLINE = 4,
        SLOW_BLINK = 5,  // might be useful for warnings?
        RAPID_BLINK = 6, // might be useful for warnings?
        SWAP_FOREGROUND_AND_BACKGROUND_COLORS = 7,
        CROSSED_OUT = 9,
    };

} // namespace tui::extras
