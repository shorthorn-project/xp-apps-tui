#include "theme/colors.hpp"
#include <string>
#include "rebuildtui.hpp"

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

    void ThemePresets::apply_minimal(NavigationTUI::Theme& theme) {
        theme.use_unicode = false;
        theme.use_colors = false;
        theme.selected_prefix = "* ";
        theme.unselected_prefix = "  ";
        theme.border_style = extras::BorderStyle::ASCII;
    }

    void ThemePresets::apply_fancy(NavigationTUI::Theme& theme) {
        theme.use_unicode = true;
        theme.use_colors = true;
        theme.selected_prefix = "✓ ";
        theme.unselected_prefix = "○ ";
        theme.border_style = extras::BorderStyle::ROUNDED;
    }

    void ThemePresets::apply_retro(NavigationTUI::Theme& theme) {
        theme.use_unicode = false;
        theme.use_colors = false;
        theme.selected_prefix = "[X] ";
        theme.unselected_prefix = "[ ] ";
        theme.border_style = extras::BorderStyle::DOUBLE;
    }

    void ThemePresets::apply_modern(NavigationTUI::Theme& theme) {
        theme.use_unicode = true;
        theme.use_colors = true;
        theme.selected_prefix = "● ";
        theme.unselected_prefix = "○ ";
        theme.border_style = extras::BorderStyle::ROUNDED;
        theme.accent_color = extras::AccentColor::BLUE;
    }

} // namespace tui::extras
