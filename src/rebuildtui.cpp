#include "rebuildtui.hpp"


#include "core/terminal.hpp"
#include "theme/colors.hpp"

#include <fmt/core.h>
#include <random>
#include <sstream>
#include <utility>

namespace tui {

    // get ANSI sequence from Color
    static std::string get_color_sequence(const extras::Color& color) {
        if (color.type == extras::Color::Type::ANSI) {
            return "\033[" + std::to_string(static_cast<int>(color.ansi_color)) + "m";
        } else if (color.type == extras::Color::Type::RGB) {
            return "\033[38;2;" + std::to_string(color.r) + ";" + std::to_string(color.g) + ";" +
                std::to_string(color.b) + "m";
        }
        return "";
    }
    NavigationTUI::NavigationTUI() :
        current_state_(NavigationState::MAIN_MENU), current_section_index_(0), current_selection_index_(0),
        current_page_(0), current_section_page_{0}, running_(false), needs_redraw_(true), previous_width_{0},
        previous_height_{0} {
        config_ = Config{};
        terminal_manager_ = std::make_unique<TerminalManager>();
    }

    NavigationTUI::NavigationTUI(Config config) :
        current_state_(NavigationState::MAIN_MENU), current_section_index_(0), current_selection_index_(0),
        current_page_(0), current_section_page_{0}, config_(std::move(config)), running_(false), needs_redraw_(true),
        previous_width_{0}, previous_height_{0} {
        terminal_manager_ = std::make_unique<TerminalManager>();
    }

    void NavigationTUI::add_section(const Section& section) { sections_.push_back(section); }

    void NavigationTUI::add_section(Section&& section) { sections_.push_back(std::move(section)); }

    void NavigationTUI::add_sections(const std::vector<Section>& sections) {
        sections_.insert(sections_.end(), sections.begin(), sections.end());
    }

    void NavigationTUI::add_sections(std::vector<Section>&& sections) {
        sections_.insert(sections_.end(), std::make_move_iterator(sections.begin()),
                         std::make_move_iterator(sections.end()));
    }

    Section* NavigationTUI::get_section(size_t index) {
        return (index < sections_.size()) ? &sections_[index] : nullptr;
    }

    const Section* NavigationTUI::get_section(size_t index) const {
        return (index < sections_.size()) ? &sections_[index] : nullptr;
    }

    Section* NavigationTUI::get_section_by_name(const std::string& name) {
#if __cplusplus >= 202002L
        const auto it =
            std::ranges::find_if(sections_, [&name](const Section& section) { return section.name == name; });
#else
        const auto it = std::find_if(sections_.begin(), sections_.end(),
                                     [&name](const Section& section) { return section.name == name; });
#endif
        return (it != sections_.end()) ? &(*it) : nullptr;
    }

    const Section* NavigationTUI::get_section_by_name(const std::string& name) const {
#if __cplusplus >= 202002L
        const auto it =
            std::ranges::find_if(sections_, [&name](const Section& section) { return section.name == name; });
#else
        const auto it = std::find_if(sections_.begin(), sections_.end(),
                                     [&name](const Section& section) { return section.name == name; });
#endif

        return (it != sections_.end()) ? &(*it) : nullptr;
    }

    size_t NavigationTUI::get_section_count() const { return sections_.size(); }

    bool NavigationTUI::remove_section(const size_t index) {
        if (index < sections_.size()) {
            using diff_t = typename decltype(sections_)::difference_type;

            sections_.erase(sections_.begin() + static_cast<diff_t>(index));
            validate_indices();
            return true;
        }
        return false;
    }

    bool NavigationTUI::remove_section_by_name(const std::string& name) {
#if __cplusplus >= 202002L
        const auto it =
            std::ranges::find_if(sections_, [&name](const Section& section) { return section.name == name; });
#else
        const auto it = std::find_if(sections_.begin(), sections_.end(),
                                     [&name](const Section& section) { return section.name == name; });
#endif

        if (it != sections_.end()) {
            sections_.erase(it);
            validate_indices();
            return true;
        }
        return false;
    }

    void NavigationTUI::clear_sections() {
        sections_.clear();
        current_section_index_ = 0;
        current_selection_index_ = 0;
        current_page_ = 0;
        current_state_ = NavigationState::MAIN_MENU;
    }

    void NavigationTUI::set_section_selected_callback(SectionSelectedCallback callback) {
        on_section_selected_ = std::move(callback);
    }

    void NavigationTUI::set_item_toggled_callback(ItemToggledCallback callback) {
        on_item_toggled_ = std::move(callback);
    }

    void NavigationTUI::set_page_changed_callback(PageChangedCallback callback) {
        on_page_changed_ = std::move(callback);
    }

    void NavigationTUI::set_state_changed_callback(StateChangedCallback callback) {
        on_state_changed_ = std::move(callback);
    }

    void NavigationTUI::set_exit_callback(ExitCallback callback) { on_exit_ = std::move(callback); }

    void NavigationTUI::set_custom_command_callback(CustomCommandCallback callback) {
        on_custom_command_ = std::move(callback);
    }

    void NavigationTUI::set_update_callback(UpdateCallback callback) { update_callback_ = std::move(callback); }

    void NavigationTUI::refresh_items() { needs_redraw_ = true; }

    void NavigationTUI::run() {
        if (sections_.empty()) {
            fmt::println("No sections available. Please add sections before running.");
            return;
        }

        initialize();
        running_ = true;

        while (running_) {
            if (needs_redraw_) {
                render();
                needs_redraw_ = false;
            }

            if (update_callback_) {
                update_callback_();
            }

            int timeout_ms = (update_callback_ != nullptr) ? 50 : 100;

            if (TerminalManager::wait_for_input(timeout_ms)) {
                process_events();
            }
        }

        terminal_manager_->restore_terminal();

        if (on_exit_) {
            on_exit_(sections_);
        }
    }

    void NavigationTUI::exit() {
        running_ = false;
        // std::fflush(stdin);
        // std::cin.clear();
    }

    NavigationTUI::NavigationState NavigationTUI::get_current_state() const { return current_state_; }

    size_t NavigationTUI::get_current_section_index() const { return current_section_index_; }

    int NavigationTUI::get_current_page() const { return current_page_; }

    size_t NavigationTUI::get_current_selection_index() const { return current_selection_index_; }

    void NavigationTUI::return_to_sections() {
        if (current_state_ != NavigationState::MAIN_MENU) {
            change_state(NavigationState::MAIN_MENU);
            current_selection_index_ = static_cast<int>(current_section_index_) % config_.layout.sections_per_page;
            current_section_page_ = static_cast<int>(current_section_index_) / config_.layout.sections_per_page;
            needs_redraw_ = true;
        }
    }

    void NavigationTUI::enter_section(const size_t section_index) {
        if (section_index < sections_.size()) {
            current_section_index_ = section_index;
            current_selection_index_ = 0;
            current_page_ = 0;
            change_state(NavigationState::ITEM_SELECTION);

            const auto& section = sections_[section_index];
            section.trigger_enter();

            if (on_section_selected_) {
                on_section_selected_(section_index, section);
            }

            needs_redraw_ = true;
        }
    }

    int NavigationTUI::get_sections_on_current_page() const {
        const int start = current_section_page_ * config_.layout.sections_per_page;
        const int end = std::min(start + config_.layout.sections_per_page, static_cast<int>(sections_.size()));
        return end - start;
    }

    void NavigationTUI::go_to_section_page(const int page) {
        if (const int total_pages = calculate_total_pages();
            page >= 0 && page < total_pages && page != current_section_page_) {
            current_section_page_ = page;
            needs_redraw_ = true;
        }
    }

    void NavigationTUI::go_to_page(const int page) {
        if (const int total_pages = calculate_total_pages(); page >= 0 && page < total_pages && page != current_page_) {
            current_page_ = page;

            if (on_page_changed_) {
                on_page_changed_(page, total_pages);
            }

            needs_redraw_ = true;
        }
    }

    void NavigationTUI::next_page() {
        if (current_state_ == NavigationState::MAIN_MENU) {
            go_to_section_page(current_section_page_ + 1);
        } else if (current_state_ == NavigationState::ITEM_SELECTION) {
            go_to_page(current_page_ + 1);
        }
    }

    void NavigationTUI::previous_page() {
        if (current_state_ == NavigationState::MAIN_MENU) {
            go_to_section_page(current_section_page_ - 1);
        } else if (current_state_ == NavigationState::ITEM_SELECTION) {
            go_to_page(current_page_ - 1);
        }
    }

    std::map<std::string, std::vector<std::string>> NavigationTUI::get_all_selections() const {
        std::map<std::string, std::vector<std::string>> selections;

        for (const auto& section : sections_) {
            if (auto selected_items = section.get_selected_names(); !selected_items.empty()) {
                selections[section.name] = selected_items;
            }
        }

        return selections;
    }

    std::vector<std::string> NavigationTUI::get_section_selections(const size_t section_index) const {
        return (section_index < sections_.size()) ? sections_[section_index].get_selected_names()
                                                  : std::vector<std::string>{};
    }

    void NavigationTUI::clear_all_selections() {
        for (auto& section : sections_) {
            section.clear_selections();
        }
        needs_redraw_ = true;
    }

    void NavigationTUI::clear_section_selections(const size_t section_index) {
        if (section_index < sections_.size()) {
            sections_[section_index].clear_selections();
            needs_redraw_ = true;
        }
    }

    void NavigationTUI::update_config(const Config& new_config) {
        config_ = new_config;
        needs_redraw_ = true;
    }

    void NavigationTUI::update_theme(const Theme& new_theme) {
        config_.theme = new_theme;
        needs_redraw_ = true;
    }

    void NavigationTUI::update_layout(const Layout& new_layout) {
        config_.layout = new_layout;
        needs_redraw_ = true;
    }

    void NavigationTUI::update_text_config(const TextConfig& new_text_config) {
        config_.text = new_text_config;
        needs_redraw_ = true;
    }

    const NavigationTUI::Config& NavigationTUI::get_config() const { return config_; }

    void NavigationTUI::initialize() {
        terminal_manager_->setup_terminal();
        validate_indices();

        auto [t_height, t_width] = TerminalManager::get_terminal_size();
        previous_width_ = t_width;
        previous_height_ = t_height;

        needs_redraw_ = true;
    }

    void NavigationTUI::process_events() {
        if (auto [t_height, t_width] = TerminalManager::get_terminal_size();
            t_width != previous_width_ || t_height != previous_height_) {
            previous_width_ = t_width;
            previous_height_ = t_height;
            needs_redraw_ = true;
        }

        // Process all pending input
        while (auto key_event = TerminalManager::get_key_input()) {
            handle_input(key_event->key, key_event->character);
        }
    }

    void NavigationTUI::handle_input(const Key key, const char character) {
        // Handle global commands first
        if (std::tolower(character) == 'q') {
            exit();
            return;
        }

        // Custom keybindings
        if (on_custom_command_ && on_custom_command_(character, current_state_)) {
            return;
        }

        // Handle state-specific input
        handle_item_input(key, character);
    }

    void NavigationTUI::handle_item_input(const Key key, const char character) {
        switch (key) {
        case Key::ESCAPE:
            return_to_sections();
            break;

        case Key::ARROW_UP:
            move_selection_up();
            break;

        case Key::ARROW_DOWN:
            move_selection_down();
            break;

        case Key::ARROW_LEFT:
            previous_page();
            break;

        case Key::ARROW_RIGHT:
            next_page();
            break;

        case Key::SPACE:
            toggle_current_item();
            break;

        case Key::ENTER:
            if (current_state_ == NavigationState::ITEM_SELECTION) {
                return_to_sections();
            } else if (current_state_ == NavigationState::MAIN_MENU) {
                select_current_item();
            }
            break;

        case Key::NORMAL:
            if (current_state_ == NavigationState::ITEM_SELECTION) {
                if (character == 'b') {
                    return_to_sections();
                } else if (character == 'a') {
                    if (current_section_index_ < sections_.size()) {
                        sections_[current_section_index_].select_all();
                        needs_redraw_ = true;
                    }
                } else if (character == 'n') {
                    if (current_section_index_ < sections_.size()) {
                        sections_[current_section_index_].clear_selections();
                        needs_redraw_ = true;
                    }
                }
            } else if (current_state_ == NavigationState::MAIN_MENU && std::isdigit(character)) {
                handle_number_input(character);
            }
            break;

        default:
            if (config_.enable_vim_keys) {
                if (character == 'j') {
                    move_selection_down();
                } else if (character == 'k') {
                    move_selection_up();
                } else if (character == 'h') {
                    return_to_sections();
                }
            }
            break;
        }
    }

    void NavigationTUI::move_selection_up() {
        if (current_state_ == NavigationState::MAIN_MENU) {
            if (current_selection_index_ > 0) {
                current_selection_index_--;
            } else if (current_section_page_ > 0) {
                go_to_section_page(current_section_page_ - 1);
                current_selection_index_ = get_sections_on_current_page() - 1;
            }
        } else {
            if (current_selection_index_ > 0) {
                current_selection_index_--;
            } else {
                if (current_page_ > 0) {
                    go_to_page(current_page_ - 1);
                    auto [first, second] = get_current_page_bounds();
                    current_selection_index_ = second - first - 1;
                }
            }
        }
        needs_redraw_ = true;
    }

    void NavigationTUI::move_selection_down() {
        if (current_state_ == NavigationState::MAIN_MENU) {
            const int items_on_page = get_sections_on_current_page();
            const int total_pages = calculate_total_pages();

            if (static_cast<int>(current_selection_index_) < items_on_page - 1) {
                current_selection_index_++;
            } else if (current_section_page_ < total_pages - 1) {
                go_to_section_page(current_section_page_ + 1);
                current_selection_index_ = 0;
            }
        } else {
            auto [first, second] = get_current_page_bounds();

            if (const size_t items_on_page = second - first; current_selection_index_ < items_on_page - 1) {
                current_selection_index_++;
            } else {
                if (const int total_pages = calculate_total_pages(); current_page_ < total_pages - 1) {
                    go_to_page(current_page_ + 1);
                    current_selection_index_ = 0;
                }
            }
        }

        needs_redraw_ = true;
    }

    void NavigationTUI::select_current_item() {
        if (current_state_ == NavigationState::MAIN_MENU) {
            size_t global_index = current_section_page_ * config_.layout.sections_per_page + current_selection_index_;
            if (global_index < sections_.size()) {
                enter_section(global_index);
            }
        } else {
            toggle_current_item();
        }
    }

    void NavigationTUI::toggle_current_item() {
        if (current_state_ == NavigationState::ITEM_SELECTION && current_section_index_ < sections_.size()) {
            auto [start, end] = get_current_page_bounds();

            if (const size_t global_index = start + current_selection_index_;
                sections_[current_section_index_].toggle_item(global_index)) {
                if (on_item_toggled_) {
                    if (const auto* item = sections_[current_section_index_].get_item(global_index)) {
                        on_item_toggled_(current_section_index_, global_index, item->selected);
                    }
                }
                needs_redraw_ = true;
            }
        }
    }

    void NavigationTUI::handle_number_input(const char digit) {
        const int number = digit - '0';

        if (current_state_ == NavigationState::MAIN_MENU) {
            if (number > 0 && number <= static_cast<int>(sections_.size())) {
                const auto global_index = number - 1;

                current_section_page_ = global_index / config_.layout.sections_per_page;
                current_selection_index_ = global_index % config_.layout.sections_per_page;

                enter_section(global_index);
            } else if (config_.layout.paginate_sections && number > 0 && number <= calculate_total_pages()) {
                go_to_section_page(number - 1);
            }
        } else if (current_state_ == NavigationState::ITEM_SELECTION && number > 0) {
            go_to_page(number - 1);
        }
    }

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
            fmt::print("{}", get_color_sequence(config_.theme.palette.border));
        }

        TerminalUtils::move_cursor(top, left);
        fmt::print("{}", top_left);
        for (auto i = 0; i < width - 2; ++i) {
            fmt::print("{}", horizontal);
        }
        fmt::print("{}", top_right);

        for (int y = top + 1; y < top + height - 1; ++y) {
            TerminalUtils::move_cursor(y, left);
            fmt::print("{}", vertical);
            TerminalUtils::move_cursor(y, left + width - 1);
            fmt::print("{}", vertical);
        }

        TerminalUtils::move_cursor(top + height - 1, left);
        fmt::print("{}", bottom_left);
        for (int i = 0; i < width - 2; ++i) {
            fmt::print("{}", horizontal);
        }
        fmt::print("{}", bottom_right);

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

    void NavigationTUI::render_header(int /*term_width*/, const int content_width, const std::string& title) {
        const std::string centered_title = center_string(title, content_width).content;
        const std::string separator = center_string(std::string(title.length(), '='), content_width).content;

        if (config_.theme.use_colors) {
            fmt::print("{}", get_color_sequence(config_.theme.palette.header_text) + centered_title);
            TerminalUtils::reset_formatting();
            fmt::println(""); // Newline from println

            fmt::print("{}", get_color_sequence(config_.theme.palette.header_border) + separator);
            TerminalUtils::reset_formatting();
            fmt::println("");
        } else {
            fmt::println("{}", centered_title);
            fmt::println("{}", separator);
        }
    }

    void NavigationTUI::apply_gradient_text(const std::string& text, const int row, const int col) const {
        if (!config_.theme.gradient_enabled || text.empty()) {
            return;
        }

        const size_t visible_len = TerminalUtils::get_visible_string_length(text);
        if (visible_len == 0) {
            TerminalUtils::move_cursor(row, col);
            fmt::print("{}", text);
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
                    fmt::print("{}", text.substr(i, end_pos - i + 1));
                    i = end_pos;
                } else {
                    fmt::print("{}", text[i]);
                }
            } else {
                if (gradient_idx < gradient.size()) {
                    TerminalUtils::set_color_rgb(gradient[gradient_idx++]);
                }
                fmt::print("{}", text[i]);
            }
        }

        TerminalUtils::reset_formatting();
    }


    void NavigationTUI::render_section_selection(const int start_row, const int left_padding, const int content_width) {
        // Header
        TerminalUtils::move_cursor(start_row, left_padding);
        fmt::print("{}", center_string(config_.text.section_selection_title, content_width).content);

        TerminalUtils::move_cursor(start_row + 1, left_padding);
        fmt::print(
            "{}",
            center_string(std::string(config_.text.section_selection_title.length(), '='), content_width).content);

        // Sections
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
                const std::string accent_color_code = get_color_sequence(config_.theme.palette.selected_item);
                fmt::print("{}{}", accent_color_code, text_to_render);
                TerminalUtils::reset_formatting();
            } else if (item.is_selected && config_.theme.gradient_enabled &&
                       config_.theme.gradient_preset != extras::GradientPreset::NONE()) {
                apply_gradient_text(text_to_render, items_start_row + static_cast<int>(i), left_padding + block_offset);
            } else {
                fmt::print("{}", text_to_render);
            }
        }
    }

    void NavigationTUI::render_item_selection(const int start_row, const int left_padding, const int content_width) {
        if (current_section_index_ >= sections_.size()) {
            return;
        }

        const auto& section = sections_[current_section_index_];

        // Header
        const std::string title = config_.text.item_selection_prefix + section.name;
        TerminalUtils::move_cursor(start_row, left_padding);
        fmt::print("{}", center_string(title, content_width).content);

        TerminalUtils::move_cursor(start_row + 1, left_padding);
        fmt::print("{}", center_string(std::string(title.length(), '='), content_width).content);

        const int items_start_row = start_row + 2 + config_.layout.vertical_padding;

        // Items
        if (section.empty()) {
            TerminalUtils::move_cursor(items_start_row, left_padding);
            fmt::print("{}", center_string(config_.text.empty_section_message, content_width).content);
            return;
        }

        size_t max_v_width = 0;

        struct ItemData {
            const SelectableItem* item;
            std::string display_text;
            bool is_selected;
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

            items.push_back({item, display_text, is_selected});
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
                    fmt::print("{}{}", get_color_sequence(config_.theme.palette.unselected_item), data.display_text);
                    TerminalUtils::reset_formatting();
                } else {
                    fmt::print("{}", data.display_text);
                }
            } else if (config_.theme.gradient_enabled &&
                       config_.theme.gradient_preset != extras::GradientPreset::NONE()) {
                apply_gradient_text(data.display_text, current_row, left_padding + block_offset);
            } else {
                if (config_.theme.use_colors) {
                    fmt::print("{}{}", get_color_sequence(config_.theme.palette.selected_item), data.display_text);
                    TerminalUtils::reset_formatting();
                } else {
                    fmt::print("{}", data.display_text);
                }
            }
        }
    }

    void NavigationTUI::render_footer(const int term_height, const int left_padding, const int content_width,
                                      const SelectableItem* item = nullptr) {
        // footer (description)
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
            fmt::print("{}", line);
            current_row++;
        }

        // footer (help text)
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
            fmt::print("{}", line);
            current_row++;
        }
    }

    std::string NavigationTUI::format_item_with_theme(const SelectableItem& item, const bool is_selected) const {
        const std::string prefix = item.selected ? config_.theme.selected_prefix : config_.theme.unselected_prefix;
        const std::string highlight = is_selected
            ? config_.theme.highlighted_prefix
            : std::string(TerminalUtils::get_visible_string_length(config_.theme.highlighted_prefix), ' ');
        std::string display_text = fmt::format("{}{} {}", highlight, prefix, item.name);

        return display_text;
    }

    std::string NavigationTUI::get_page_info_string() const {
        int total_pages = calculate_total_pages();
        return fmt::format(
            "Page {} of {}",
            (current_state_ == NavigationState::MAIN_MENU) ? current_section_page_ + 1 : current_page_ + 1,
            total_pages);
    }

    int NavigationTUI::calculate_total_pages() const {
        if (current_state_ == NavigationState::MAIN_MENU) {
            return (!config_.layout.paginate_sections || sections_.empty())
                ? 1
                : (static_cast<int>((sections_.size() + config_.layout.sections_per_page - 1)) /
                   config_.layout.sections_per_page);
        }

        if (current_section_index_ < sections_.size()) {
            const size_t item_count = sections_[current_section_index_].size();
            if (item_count == 0) {
                return 1;
            }
            return static_cast<int>((item_count + config_.layout.items_per_page - 1) / config_.layout.items_per_page);
        }

        return 1;
    }

    std::pair<size_t, size_t> NavigationTUI::get_current_page_bounds() const {
        if (current_state_ != NavigationState::ITEM_SELECTION || current_section_index_ >= sections_.size()) {
            return {0, 0};
        }

        size_t start = current_page_ * config_.layout.items_per_page;
        size_t end = std::min(start + config_.layout.items_per_page, sections_[current_section_index_].size());

        return {start, end};
    }

    void NavigationTUI::clamp_selection() {
        if (current_state_ == NavigationState::MAIN_MENU && current_section_index_ >= sections_.size()) {
            current_selection_index_ = !sections_.empty() ? sections_.size() - 1 : 0;
        } else {
            auto [first, second] = get_current_page_bounds();
            if (const size_t max_selection = second - first; current_selection_index_ >= max_selection) {
                current_selection_index_ = max_selection > 0 ? max_selection - 1 : 0;
            }
        }
    }

    void NavigationTUI::change_state(const NavigationState new_state) {
        if (current_state_ != new_state) {
            const NavigationState old_state = current_state_;
            current_state_ = new_state;

            if (on_state_changed_) {
                on_state_changed_(old_state, new_state);
            }
        }
    }

    void NavigationTUI::validate_indices() {
        if (current_section_index_ >= sections_.size()) {
            current_section_index_ = !sections_.empty() ? sections_.size() - 1 : 0;
        }
        clamp_selection();
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

    NavigationBuilder& NavigationBuilder::theme_indicators(const char selected, const char unselected) {
        config_.theme.selected_indicator = selected;
        config_.theme.unselected_indicator = unselected;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_prefixes(const std::string& selected, const std::string& unselected,
                                                         const std::string& highlighted) {
        config_.theme.selected_prefix = selected;
        config_.theme.unselected_prefix = unselected;
        config_.theme.highlighted_prefix = highlighted;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_unicode(const bool enable) {
        config_.theme.use_unicode = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_colors(const bool enable) {
        config_.theme.use_colors = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_gradient_support(const bool enable) {
        config_.theme.gradient_enabled = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_gradient_preset(const extras::GradientPreset& preset) {
        config_.theme.gradient_preset = preset;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_gradient_randomize(const bool enable) {
        config_.theme.gradient_randomize = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_border_style(const extras::BorderStyle& style) {
        config_.theme.border_style = style;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_accent_color(const extras::AccentColor& color) {
        config_.theme.accent_color = color;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_palette(const extras::ColorPalette& palette) {
        config_.theme.palette = palette;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_color(const std::string& element, const extras::Color& color) {
        if (element == "border") {
            config_.theme.palette.border = color;
        } else if (element == "header_text") {
            config_.theme.palette.header_text = color;
        } else if (element == "header_border") {
            config_.theme.palette.header_border = color;
        } else if (element == "section_name") {
            config_.theme.palette.section_name = color;
        } else if (element == "item_name") {
            config_.theme.palette.item_name = color;
        } else if (element == "selected_item") {
            config_.theme.palette.selected_item = color;
        } else if (element == "unselected_item") {
            config_.theme.palette.unselected_item = color;
        } else if (element == "counter") {
            config_.theme.palette.counter = color;
        } else if (element == "footer") {
            config_.theme.palette.footer = color;
        }
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_centering(const bool horizontal, const bool vertical) {
        config_.layout.center_horizontally = horizontal;
        config_.layout.center_vertically = vertical;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_content_width(const int min_width, const int max_width) {
        config_.layout.min_content_width = min_width;
        config_.layout.max_content_width = max_width;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_padding(const int vertical_padding) {
        config_.layout.vertical_padding = vertical_padding;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_auto_resize(const bool enable) {
        config_.layout.auto_resize_content = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_borders(const bool show) {
        config_.layout.show_borders = show;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_items_per_page(const int count) {
        config_.layout.items_per_page = count;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_sections_per_page(const int count) {
        config_.layout.sections_per_page = count;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::paginate_sections(const bool paginate) {
        config_.layout.paginate_sections = paginate;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::text_titles(const std::string& section_title,
                                                      const std::string& item_prefix) {
        config_.text.section_selection_title = section_title;
        config_.text.item_selection_prefix = item_prefix;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::text_messages(const std::string& empty_message) {
        config_.text.empty_section_message = empty_message;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::text_help(const std::string& section_help, const std::string& item_help) {
        config_.text.help_text_sections = section_help;
        config_.text.help_text_items = item_help;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::text_show_help(const bool show) {
        config_.text.show_help_text = show;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::text_show_pages(const bool show) {
        config_.text.show_page_numbers = show;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::text_show_counters(const bool show) {
        config_.text.show_counters = show;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::keys_quick_select(const bool enable) {
        config_.enable_quick_select = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::keys_vim_style(const bool enable) {
        config_.enable_vim_keys = enable;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::keys_custom_shortcut(const char key, const std::string& description) {
        config_.custom_shortcuts[key] = description;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::add_section(const Section& section) {
        sections_.push_back(section);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::add_section(Section&& section) {
        sections_.push_back(std::move(section));
        return *this;
    }

    NavigationBuilder& NavigationBuilder::add_sections(const std::vector<Section>& sections) {
        sections_.insert(sections_.end(), sections.begin(), sections.end());
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_section_selected(NavigationTUI::SectionSelectedCallback callback) {
        section_selected_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_item_toggled(NavigationTUI::ItemToggledCallback callback) {
        item_toggled_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_page_changed(NavigationTUI::PageChangedCallback callback) {
        page_changed_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_state_changed(NavigationTUI::StateChangedCallback callback) {
        state_changed_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_exit(NavigationTUI::ExitCallback callback) {
        exit_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_custom_command(NavigationTUI::CustomCommandCallback callback) {
        custom_command_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::on_update(NavigationTUI::UpdateCallback callback) {
        update_callback_ = std::move(callback);
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_minimal() {
        config_.theme.use_unicode = false;
        config_.theme.use_colors = false;
        config_.theme.selected_prefix = "* ";
        config_.theme.unselected_prefix = "  ";
        config_.theme.border_style = extras::BorderStyle::ASCII;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_fancy() {
        config_.theme.use_unicode = true;
        config_.theme.use_colors = true;
        config_.theme.selected_prefix = "✓ ";
        config_.theme.unselected_prefix = "○ ";
        config_.theme.border_style = extras::BorderStyle::ROUNDED;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_retro() {
        config_.theme.use_unicode = false;
        config_.theme.use_colors = false;
        config_.theme.selected_prefix = "[X] ";
        config_.theme.unselected_prefix = "[ ] ";
        config_.theme.border_style = extras::BorderStyle::DOUBLE;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::theme_modern() {
        config_.theme.use_unicode = true;
        config_.theme.use_colors = true;
        config_.theme.selected_prefix = "● ";
        config_.theme.unselected_prefix = "○ ";
        config_.theme.border_style = extras::BorderStyle::ROUNDED;
        config_.theme.accent_color = extras::AccentColor::BLUE;

        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_compact() {
        config_.layout.items_per_page = 25;
        config_.layout.show_borders = false;
        config_.layout.center_horizontally = false;
        config_.layout.center_vertically = false;
        config_.layout.min_content_width = 40;
        config_.layout.max_content_width = 60;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_comfortable() {
        config_.layout.items_per_page = 15;
        config_.layout.show_borders = true;
        config_.layout.center_horizontally = false;
        config_.layout.center_vertically = false;
        config_.layout.min_content_width = 60;
        config_.layout.max_content_width = 100;
        config_.layout.vertical_padding = 2;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_fullscreen() {
        config_.layout.items_per_page = 30;
        config_.layout.show_borders = true;
        config_.layout.center_horizontally = false;
        config_.layout.center_vertically = false;
        config_.layout.auto_resize_content = true;
        config_.layout.min_content_width = 80;
        config_.layout.max_content_width = 120;
        return *this;
    }

    NavigationBuilder& NavigationBuilder::layout_centered() {
        config_.layout.center_horizontally = true;
        config_.layout.center_vertically = false;
        config_.layout.items_per_page = 20;
        config_.layout.show_borders = true;
        config_.layout.min_content_width = 60;
        config_.layout.max_content_width = 80;
        config_.layout.vertical_padding = 3;
        return *this;
    }

    std::unique_ptr<NavigationTUI> NavigationBuilder::build() {
        auto tui = std::make_unique<NavigationTUI>(config_);

        for (auto& section : sections_) {
            tui->add_section(std::move(section));
        }

        if (section_selected_callback_) {
            tui->set_section_selected_callback(section_selected_callback_);
        }
        if (item_toggled_callback_) {
            tui->set_item_toggled_callback(item_toggled_callback_);
        }
        if (page_changed_callback_) {
            tui->set_page_changed_callback(page_changed_callback_);
        }
        if (state_changed_callback_) {
            tui->set_state_changed_callback(state_changed_callback_);
        }
        if (exit_callback_) {
            tui->set_exit_callback(exit_callback_);
        }
        if (custom_command_callback_) {
            tui->set_custom_command_callback(custom_command_callback_);
        }
        if (update_callback_) {
            tui->set_update_callback(update_callback_);
        }

        return tui;
    }

    const NavigationTUI::Config& NavigationBuilder::get_config() const { return config_; }

    NavigationBuilder& NavigationBuilder::reset() {
        config_ = NavigationTUI::Config{};
        sections_.clear();

        section_selected_callback_ = nullptr;
        item_toggled_callback_ = nullptr;
        page_changed_callback_ = nullptr;
        state_changed_callback_ = nullptr;
        exit_callback_ = nullptr;
        custom_command_callback_ = nullptr;
        update_callback_ = nullptr;

        return *this;
    }
} // namespace tui
