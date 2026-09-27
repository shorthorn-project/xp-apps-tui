#include <algorithm>
#include <fmt/core.h>
#include <random>
#include <sstream>
#include "core/terminal.hpp"
#include "rebuildtui.hpp"
#include "theme/colors.hpp"

#ifdef _WIN32
#define TUI_PRINT(...) TerminalUtils::print_safe(fmt::format(__VA_ARGS__))
#else
#define TUI_PRINT(...) fmt::print(__VA_ARGS__)
#endif

namespace tui {

    int NavigationTUI::get_effective_content_width(const int term_width) const {
        int content_width = term_width - 4;
        if (config_.layout.show_borders) {
            content_width -= 2;
        }
        content_width = (config_.layout.auto_resize_content)
            ? std::clamp(content_width, config_.layout.min_content_width, config_.layout.max_content_width)
            : config_.layout.max_content_width;
        return content_width;
    }

    int NavigationTUI::get_effective_content_height() const {
        auto content_height = 0;
        if (current_state_ == NavigationState::MAIN_MENU) {
            content_height = 3 + static_cast<int>(sections_.size()) + 2;
        } else if (current_section_index_ < sections_.size()) {
            auto [first, second] = get_current_page_bounds();
            content_height = 3 + static_cast<int>((second - first)) + 2;
        }
        content_height += 2 * config_.layout.vertical_padding;
        if (config_.layout.show_borders) {
            content_height += 2;
        }
        return content_height;
    }

    void NavigationTUI::draw_border(int top, int left, int width, int height) const {
        std::string top_left, top_right, bottom_left, bottom_right, horizontal, vertical;

        switch (config_.theme.border_style) {
        case extras::BorderStyle::ROUNDED:
            top_left = "╭";
            top_right = "╮";
            bottom_left = "╰";
            bottom_right = "╯";
            horizontal = "─";
            vertical = "│";
            break;
        case extras::BorderStyle::DOUBLE:
            top_left = "╔";
            top_right = "╗";
            bottom_left = "╚";
            bottom_right = "╝";
            horizontal = "═";
            vertical = "║";
            break;
        case extras::BorderStyle::SHARP:
            top_left = "┌";
            top_right = "┐";
            bottom_left = "└";
            bottom_right = "┘";
            horizontal = "─";
            vertical = "│";
            break;
        case extras::BorderStyle::ASCII:
        default:
            top_left = "+";
            top_right = "+";
            bottom_left = "+";
            bottom_right = "+";
            horizontal = "-";
            vertical = "|";
            break;
        }

        if (config_.theme.use_colors) {
            TUI_PRINT("{}", extras::get_color_sequence(config_.theme.palette.border));
        }

        TerminalUtils::move_cursor(top, left);
        TUI_PRINT("{}", top_left);
        for (auto i = 0; i < width - 2; ++i) {
            TUI_PRINT("{}", horizontal);
        }
        TUI_PRINT("{}", top_right);

        for (int y = top + 1; y < top + height - 1; ++y) {
            TerminalUtils::move_cursor(y, left);
            TUI_PRINT("{}", vertical);
            TerminalUtils::move_cursor(y, left + width - 1);
            TUI_PRINT("{}", vertical);
        }

        TerminalUtils::move_cursor(top + height - 1, left);
        TUI_PRINT("{}", bottom_left);
        for (int i = 0; i < width - 2; ++i) {
            TUI_PRINT("{}", horizontal);
        }
        TUI_PRINT("{}", bottom_right);

        TerminalUtils::reset_formatting();
    }

    void NavigationTUI::render() {
        if (!needs_redraw_) {
            return;
        }

        TerminalManager::clear_screen();

        auto [term_height, term_width] = TerminalManager::get_terminal_size();
        int content_width = get_effective_content_width(term_width);
        auto left_padding = 1;

        if (config_.layout.center_horizontally) {
            left_padding = (term_width - content_width) / 2;
        }

        auto start_row = 1;
        if (config_.layout.center_vertically) {
            const int content_height = get_effective_content_height();
            start_row = std::max(1, (term_height - content_height) / 2);
        }

        if (config_.layout.show_borders) {
            content_width = std::max(10, content_width - 2);
            left_padding = std::max(1, left_padding - 1);
            start_row = std::max(1, start_row - 1);
        }

        if (config_.layout.show_borders) {
            auto content_height = 0;
            if (current_state_ == NavigationState::MAIN_MENU) {
                content_height = 3 + static_cast<int>(sections_.size()) + 2;
            } else if (current_section_index_ < sections_.size()) {
                auto [first, second] = get_current_page_bounds();
                content_height = 3 + static_cast<int>((second - first)) + 2;
            }
            content_height += 2 * config_.layout.vertical_padding;
            draw_border(start_row, left_padding, content_width + 2, content_height + 2);

            left_padding += 1;
            start_row += 1;
        }

        start_row += config_.layout.vertical_padding;

        if (current_state_ == NavigationState::MAIN_MENU) {
            render_section_selection(start_row, left_padding, content_width);
        } else {
            render_item_selection(start_row, left_padding, content_width);
        }

        const SelectableItem* current_item = nullptr;
        if (current_state_ == NavigationState::ITEM_SELECTION && current_section_index_ < sections_.size()) {
            const auto& section = sections_[current_section_index_];
            if (auto [first, second] = get_current_page_bounds(); current_selection_index_ < (second - first)) {
                const size_t global_index = first + current_selection_index_;
                current_item = section.get_item(global_index);
            }
        }

        render_footer(term_height, left_padding, content_width, current_item);
        TerminalManager::flush_output();
        needs_redraw_ = false;
    }

    void NavigationTUI::render_header(int, const int content_width, const std::string& title) {
        const std::string centered_title = center_string(title, content_width).content;
        const std::string separator = center_string(std::string(title.length(), '='), content_width).content;

        if (config_.theme.use_colors) {
            TUI_PRINT("{}", extras::get_color_sequence(config_.theme.palette.header_text) + centered_title);
            TerminalUtils::reset_formatting();
            TUI_PRINT("\n");
            TUI_PRINT("{}", extras::get_color_sequence(config_.theme.palette.header_border) + separator);
            TerminalUtils::reset_formatting();
            TUI_PRINT("\n");
        } else {
            TUI_PRINT("{}\n", centered_title);
            TUI_PRINT("{}\n", separator);
        }
    }

    void NavigationTUI::apply_gradient_text(const std::string& text, const int row, const int col) const {
        if (!config_.theme.gradient_enabled || text.empty()) {
            return;
        }

#ifdef _WIN32
        if (!TerminalUtils::is_vt_supported()) {
            TerminalUtils::move_cursor(row, col);
            TUI_PRINT("{}", text);
            return;
        }
#endif

        const size_t visible_len = TerminalUtils::get_visible_string_length(text);
        if (visible_len == 0) {
            TerminalUtils::move_cursor(row, col);
            TUI_PRINT("{}", text);
            return;
        }

        auto gradient =
            extras::GradientColor::from_preset(config_.theme.gradient_preset, static_cast<int>(visible_len));
        if (config_.theme.gradient_randomize) {
#if __cplusplus >= 202002L
            std::ranges::shuffle(gradient, std::mt19937(std::random_device()()));
#else
            std::shuffle(gradient.begin(), gradient.end(), std::mt19937(std::random_device()()));
#endif
        }

        TerminalUtils::move_cursor(row, col);

        size_t gradient_idx = 0;
        for (size_t i = 0; i < text.length(); ++i) {
            if (text[i] == '\033') {
                if (const size_t end_pos = text.find('m', i); end_pos != std::string::npos) {
                    TUI_PRINT("{}", text.substr(i, end_pos - i + 1));
                    i = end_pos;
                } else {
                    TUI_PRINT("{}", text[i]);
                }
            } else {
                if (gradient_idx < gradient.size()) {
                    TerminalUtils::set_color_rgb(gradient[gradient_idx++]);
                }
                TUI_PRINT("{}", text[i]);
            }
        }
        TerminalUtils::reset_formatting();
    }

    void NavigationTUI::render_section_selection(const int start_row, const int left_padding, const int content_width) {
        TerminalUtils::move_cursor(start_row, left_padding);
        TUI_PRINT("{}", center_string(config_.text.section_selection_title, content_width).content);

        TerminalUtils::move_cursor(start_row + 1, left_padding);
        TUI_PRINT(
            "{}",
            center_string(std::string(config_.text.section_selection_title.length(), '='), content_width).content);

        const auto start_index = current_section_page_ * config_.layout.sections_per_page;
        const auto end_index =
            std::min(start_index + config_.layout.sections_per_page, static_cast<int>(sections_.size()));
        const auto items_on_page = end_index - start_index;
        const int items_start_row = start_row + 2 + config_.layout.vertical_padding;

        const size_t highlight_v_w = TerminalUtils::get_visible_string_length(config_.theme.highlighted_prefix);

        struct SectionItem {
            std::string base_text;
            bool is_selected;
            size_t v_width;
        };
        std::vector<SectionItem> items_to_render;
        size_t max_v_width = 0;

        for (auto i = 0; i < items_on_page; ++i) {
            const size_t global_index = start_index + i;
            const bool is_selected = i == static_cast<int>(current_selection_index_);

            std::string display_text = fmt::format("{}. {}", global_index + 1, sections_[global_index].name);
            if (config_.text.show_counters) {
                const size_t selected_count = sections_[global_index].get_selected_count();
                if (const size_t total_count = sections_[global_index].size(); total_count > 0) {
                    display_text += " (" + std::to_string(selected_count) + "/" + std::to_string(total_count) + ")";
                }
            }

            const size_t total_v_w = highlight_v_w + TerminalUtils::get_visible_string_length(display_text);
            if (total_v_w > max_v_width) {
                max_v_width = total_v_w;
            }
            items_to_render.push_back({display_text, is_selected, total_v_w});
        }

        const int block_offset = (config_.layout.center_horizontally && content_width > static_cast<int>(max_v_width))
            ? (content_width - static_cast<int>(max_v_width)) / 2
            : 0;

        for (size_t i = 0; i < items_to_render.size(); ++i) {
            const auto& item = items_to_render[i];
            const std::string highlight =
                item.is_selected ? config_.theme.highlighted_prefix : std::string(highlight_v_w, ' ');
            const std::string text_to_render = highlight + item.base_text;
            TerminalUtils::move_cursor(items_start_row + static_cast<int>(i), left_padding + block_offset);

            if (item.is_selected && config_.theme.use_colors &&
                !(config_.theme.gradient_enabled && config_.theme.gradient_preset != extras::GradientPreset::NONE())) {
                TUI_PRINT("{}{}", extras::get_color_sequence(config_.theme.palette.selected_item), text_to_render);
                TerminalUtils::reset_formatting();
            } else if (item.is_selected && config_.theme.gradient_enabled &&
                       config_.theme.gradient_preset != extras::GradientPreset::NONE()) {
                apply_gradient_text(text_to_render, items_start_row + static_cast<int>(i), left_padding + block_offset);
            } else {
                TUI_PRINT("{}", text_to_render);
            }
        }
    }

    void NavigationTUI::render_item_selection(const int start_row, const int left_padding, const int content_width) {
        if (current_section_index_ >= sections_.size()) {
            return;
        }

        const auto& section = sections_[current_section_index_];
        const std::string title = config_.text.item_selection_prefix + section.name;
        TerminalUtils::move_cursor(start_row, left_padding);
        TUI_PRINT("{}", center_string(title, content_width).content);

        TerminalUtils::move_cursor(start_row + 1, left_padding);
        TUI_PRINT("{}", center_string(std::string(title.length(), '='), content_width).content);

        const int items_start_row = start_row + 2 + config_.layout.vertical_padding;

        if (section.empty()) {
            TerminalUtils::move_cursor(items_start_row, left_padding);
            TUI_PRINT("{}", center_string(config_.text.empty_section_message, content_width).content);
            return;
        }

        size_t max_v_width = 0;
        struct ItemData {
            const SelectableItem* item;
            std::string display_text;
            bool is_selected;

            ItemData(const SelectableItem* i, std::string d, bool s) :
                item(i), display_text(std::move(d)), is_selected(s) {}
        };
        std::vector<ItemData> items;

        auto [first, second] = get_current_page_bounds();
        for (size_t i = first; i < second; ++i) {
            const auto* item = section.get_item(i);
            if (!item) {
                continue;
            }

            const bool is_selected = (i - first) == current_selection_index_;
            std::string display_text = format_item_with_theme(*item, is_selected);
            size_t v_width = TerminalUtils::get_visible_string_length(display_text);

            if (v_width > max_v_width) {
                max_v_width = v_width;
            }
            items.emplace_back(item, display_text, is_selected);
        }

        const int block_offset = (config_.layout.center_horizontally && content_width > static_cast<int>(max_v_width))
            ? (content_width - static_cast<int>(max_v_width)) / 2
            : 0;

        for (size_t i = 0; i < items.size(); ++i) {
            const auto& data = items[i];
            const int current_row = static_cast<int>(items_start_row + i);
            TerminalUtils::move_cursor(current_row, left_padding + block_offset);

            if (!data.is_selected) {
                if (config_.theme.use_colors) {
                    TUI_PRINT("{}{}", extras::get_color_sequence(config_.theme.palette.unselected_item),
                              data.display_text);
                    TerminalUtils::reset_formatting();
                } else {
                    TUI_PRINT("{}", data.display_text);
                }
            } else if (config_.theme.gradient_enabled &&
                       config_.theme.gradient_preset != extras::GradientPreset::NONE()) {
                apply_gradient_text(data.display_text, current_row, left_padding + block_offset);
            } else {
                if (config_.theme.use_colors) {
                    TUI_PRINT("{}{}", extras::get_color_sequence(config_.theme.palette.selected_item),
                              data.display_text);
                    TerminalUtils::reset_formatting();
                } else {
                    TUI_PRINT("{}", data.display_text);
                }
            }
        }
    }

    void NavigationTUI::render_footer(const int term_height, const int left_padding, const int content_width,
                                      const SelectableItem* item) {
        std::string description;

        if (item) {
            description = item->description.empty() ? "No description provided" : item->description;
        } else if (current_state_ == NavigationState::MAIN_MENU) {
            const auto start_index = current_section_page_ * config_.layout.sections_per_page;
            const size_t global_index = start_index + current_selection_index_;

            if (global_index < sections_.size()) {
                const auto& section = sections_[global_index];
                description = section.description.empty() ? "No description provided" : section.description;
            } else {
                description = "No section selected";
            }
        } else {
            description = "Description (placeholder)";
        }

        auto [content, line_count] = center_string(description, content_width);

        const int description_anchor_row = term_height - 4;
        const int description_start_row = description_anchor_row - (line_count - 1);

        TerminalUtils::move_cursor(description_start_row, left_padding);

        std::istringstream stream(content);
        std::string line;
        int current_row = description_start_row;

        while (std::getline(stream, line)) {
            TerminalUtils::move_cursor(current_row, left_padding);
            TUI_PRINT("{}", line);
            current_row++;
        }

        std::string help_text = (current_state_ == NavigationState::MAIN_MENU) ? config_.text.help_text_sections
                                                                               : config_.text.help_text_items;
        if ((current_state_ == NavigationState::MAIN_MENU && config_.layout.paginate_sections &&
             config_.text.show_page_numbers) ||
            (current_state_ == NavigationState::ITEM_SELECTION && config_.text.show_page_numbers)) {
            help_text += " | " + get_page_info_string();
        }

        auto [help_content, help_line_count] = center_string(help_text, content_width);

        const int help_anchor_row = term_height - 2;
        const int help_start_row = help_anchor_row - (help_line_count - 1);

        TerminalUtils::move_cursor(help_start_row, left_padding);

        current_row = help_start_row;
        std::istringstream help_stream(help_content);
        while (std::getline(help_stream, line)) {
            TerminalUtils::move_cursor(current_row, left_padding);
            TUI_PRINT("{}", line);
            current_row++;
        }
    }

    std::string NavigationTUI::format_item_with_theme(const SelectableItem& item, const bool is_selected) const {
        const std::string prefix = item.selected ? config_.theme.selected_prefix : config_.theme.unselected_prefix;
        const std::string highlight = is_selected
            ? config_.theme.highlighted_prefix
            : std::string(TerminalUtils::get_visible_string_length(config_.theme.highlighted_prefix), ' ');
        return fmt::format("{}{} {}", highlight, prefix, item.name);
    }

    std::string NavigationTUI::get_page_info_string() const {
        int total_pages = calculate_total_pages();
        return fmt::format(
            "Page {} of {}",
            (current_state_ == NavigationState::MAIN_MENU) ? current_section_page_ + 1 : current_page_ + 1,
            total_pages);
    }

    NavigationTUI::FormattedText NavigationTUI::center_string(const std::string& text, const int width) const {
        if (!config_.layout.center_horizontally) {
            int lines = text.empty() ? 0 : 1;
            for (char c : text) {
                if (c == '\n') {
                    lines++;
                }
            }
            return {text, lines};
        }

        std::stringstream final_text_stream;
        std::stringstream input_stream(text);
        std::string line;
        auto total_lines = 0;
        auto first_line = true;

        while (std::getline(input_stream, line)) {
            if (!first_line) {
                final_text_stream << '\n';
            }

            const size_t visible_len = TerminalUtils::get_visible_string_length(line);
            auto padding = 0;
            if (width > static_cast<int>(visible_len)) {
                padding = static_cast<int>(width - visible_len) / 2;
            }

            if (padding > 0) {
                final_text_stream << std::string(padding, ' ');
            }
            final_text_stream << line;
            total_lines++;
            first_line = false;
        }

        if (text.empty()) {
            return {"", 0};
        }
        if (!text.empty() && text.back() == '\n') {
            final_text_stream << '\n';
            total_lines++;
        }
        return {final_text_stream.str(), total_lines};
    }

} // namespace tui
